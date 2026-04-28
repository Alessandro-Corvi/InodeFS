#include "fs.h"
#include "aux.h"

Filesystem *fs;

int format_fs(const char* filename, int fs_size) {
    int fd = open(filename, O_RDWR | O_CREAT, 0644);
    if (fd == -1) {
        printf("Errore: impossibile creare il file %s\n", filename);
        return -1;
    }

    if (ftruncate(fd, fs_size) == -1) {
        printf("Errore: impossibile impostare la dimensione del file %s\n", filename);
        close(fd);
        return -1;
    }

    char *ptr = mmap(NULL, fs_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        printf("Errore: impossibile mappare il file %s\n", filename);
        close(fd);
        return -1;
    }

    //Inizializzo la memoria mappata a zero
    memset(ptr, 0, fs_size);

    // Inizializza la struttura del file system
    fs=(Filesystem*)malloc(sizeof(Filesystem));
    
    // Inizializza il file system (ad esempio, scrivi la superblock)
    fs->sb =(Superblock*)ptr;
    fs->sb->fs_size = fs_size;
    fs->sb->block_size = 512; 
    fs->sb->num_blocks = fs_size / fs->sb->block_size;
    fs->sb->num_inodes = 128; 
    fs->sb->free_blocks = fs->sb->num_blocks;
    fs->sb->free_inodes = fs->sb->num_inodes;

    fs->data_bitmap = ptr + sizeof(Superblock);
    fs->inodes = (Inode*)(fs->data_bitmap + fs->sb->num_blocks / 8);
    fs->data = (char*)(fs->inodes + fs->sb->num_inodes);

    //Inizializza inode table
    for(int i=0; i < fs->sb->num_inodes; i++){
        Inode *inode = (Inode*) &(fs->inodes[i]);
        inode->id=i;
    }
    //Inizializzo Root
    Inode *inode = &fs->inodes[0];
    inode->used = 1;
    fs->current_dir = inode;
    fs->current_path[0] = '/';


    //Deve puntare a se stessa in questo caso
    //Nel caso di creazione cartella mkdir, il puntatore ".." deve puntare alla cartella padre
    //e per farlo devo fs->current_dir->block_ptrs[0] = block_index della cartella 
    //quindi mi conviene aggiungere direttamente l'id all'inode e usarlo per identificare la cartella padre

    // Sincronizza i dati su disco
    syncFS();
    return 0;
}


int load_fs(const char* filename) {
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("Errore: impossibile aprire il file %s\n", filename);
        return -1;
    }

    //Ottengo la lunghezza del file
    struct stat st;
    if (fstat(fd, &st) == -1) {
        printf("Errore: impossibile ottenere le informazioni sul file %s\n", filename);
        close(fd);
        return -1;  
    }

    char *ptr = mmap(NULL, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        printf("Errore: impossibile mappare il file %s\n", filename);
        close(fd);
        return -1; 
    }

    // Inizializza la struttura del file system
    fs=(Filesystem*)malloc(sizeof(Filesystem));
    fs->sb =(Superblock*)ptr;
    fs->data_bitmap = ptr + sizeof(Superblock);
    fs->inodes = (Inode*)(fs->data_bitmap + fs->sb->num_blocks / 8);
    fs->data = (char*)(fs->inodes + fs->sb->num_inodes);
    fs->current_dir = fs->inodes;
    fs->current_path[0] = '/';  
    return 0;
}


int create_dir(const char* dirname) {
    //Controllo che non esiste già
    printf("Ciao1");
    if(findEntry(fs->current_dir,dirname) != NULL){
          printf("È già presente una cartella con il nome %s",dirname);
          return -1;
    }

     printf("Ciao2");
    //Cerca inode libero
    Inode *new_inode = search_free_inode();
    if(new_inode == NULL){
        printf("Tabella degli inode piena");
        return -1;
    }

    //Inizializza l'inode
    new_inode->is_dir=1;
    
    //Aggiunge le entry . e ..
    int block_index = search_free_block();
    if(block_index < 0){
        printf("Spazio dei blocchi esaurito");
        return -1;
    }
    /*
        NOTA: se volessi aggiungere . e .. con add_dir_entry
        devo mettere come parametri d'ingresso anche l'inode
    */

    //Aggiunge le entry . e ..
    add_dir_entry(new_inode,".",new_inode->id);
    add_dir_entry(new_inode,"..",fs->current_dir->id);
    

    //Aggiunge l'entry al padre
    add_dir_entry(fs->current_dir,dirname,new_inode->id);

    syncFS();
    printf("\nDirectory %s salvata correttamente\n", dirname);
    return 0;
}


void print_dir(int option){
    if(option){
        printf("Inode-Name\n");
    }
    for (int i=0; i<NUM_PTRS; i++){
        
        char *block = (char*)get_inode_block(fs->current_dir,i);
        if(block==NULL){continue;}

        DirEntry *entries = (DirEntry*) block;
        for(int j=0; j<ENTRIES_PER_BLOCK; j++){
            
            if(option){
                if(entries[j].name[0] == '\0'){continue;}
                printf("%d  %s\n",entries[j].id,entries[j].name);
            }
            else{
                if(entries[j].name[0] == '\0'){continue;}
                printf("%s\n",entries[j].name);
            }
        }

    }
}


void change_dir(const char *dirname) {
    if (strcmp(".", dirname) == 0) {
        return;
    } else if (strcmp("..", dirname) == 0) {
        if (strcmp(fs->current_path, "/") == 0) {
            return; // già alla root
        }
        // aggiorna il path: tronca all'ultimo /
        char *last_slash = strrchr(fs->current_path, '/');
        if (last_slash == fs->current_path) {
            *(last_slash + 1) = '\0'; // torna a "/"
        } else {
            *last_slash = '\0'; // rimuove ultimo componente
        }
        
        DirEntry *entry = (DirEntry*)(fs->data + fs->current_dir->direct[0] * fs->sb->block_size);
        fs->current_dir = &fs->inodes[entry[1].id];
    } else {
        // directory normale
        DirEntry *entry= findEntry(fs->current_dir,dirname);
        if (entry == NULL) {
            printf("Directory non trovata: %s\n", dirname);
            return;
        }
        Inode *inode = &fs->inodes[entry->id];
        if (!inode->is_dir) {
            printf("%s non è una directory\n", dirname);
            return;
        }
        // aggiorna path
        if (strcmp(fs->current_path, "/") != 0) {
            strcat(fs->current_path, "/");
        }
        strcat(fs->current_path, dirname);
        // aggiorna current_dir
        fs->current_dir = inode;
    }
}

int remove_dir(const char* dirname){
    DirEntry *entry = findEntry(fs->current_dir,dirname);
    if(entry == NULL){
        printf("Non è presente la cartella con nome %s", dirname);
        return -1;
    }


    Inode *inode = (Inode*) &(fs->inodes[entry->id]);
    if(inode->num_entries > 2){
        printf("La cartella non è vuota");
        return -1;
    }
    /*
    Non aggiungo il controllo inode->is_dir = 1
    perchè già l'ho fatto nella shell, cioè non 
    permette la creazione di cartelle con il punto
    */

    //Rimuovo le entry . e ..
    DirEntry *entries = (DirEntry*)(fs->data  + inode->direct[0] * fs->sb->block_size);
    memset(entries,0,2*sizeof(DirEntry));

    //Rimuovo la entry dal padre
    remove_dir_entry(fs->current_dir,inode->id);
    syncFS();
    return 0;
}



int create_file(const char* filename){
    if(fs->current_dir->num_entries >= MAX_ENTRIES){
        printf("Directory attuale piena.");
        return -1;
    }

    if(findEntry(fs->current_dir,filename) != NULL){
        printf("File con nome %s già presente ",filename);
        return -1;
    }

    //Cerco un inode libero per il file
    Inode *new_inode = search_free_inode();
    if(new_inode == NULL){
        printf("Tabella degli inode piena");
        return -1;
    }

    new_inode->is_dir = 0;
    new_inode->used = 1;

    //Aggiungo la entry alla directory padre
    add_dir_entry(fs->current_dir,filename,new_inode->id);

    //Apporto le modifiche al disco
    syncFS();
    return 0;
}

/*
ReadFile uso le funzioni
*/