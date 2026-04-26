#include "fs.h"
#include "aux.h"

Filesystem *fs;

int formatFS(const char* filename, int fs_size) {
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
    memset(fs,0,sizeof(Filesystem));
    
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
        inode->is_dir=0;
        inode->size=0;
        inode->used=0;
        inode->num_entries = 0;
        for(int i=0; i < 12; i++){
            inode->direct[i] = NULL_PTR;
        }
        inode->indirect = NULL_PTR;
    }


    Inode *inode = &fs->inodes[0];
    inode->used = 1;
    fs->current_dir = inode;
    fs->current_path[0] = '/';

    int block_index = searchFreeBlock();
    if(block_index < 0){
        printf("Spazio dei blocchi esaurito");
        return -1;
    }
    fs->current_dir->direct[0] = block_index;
    DirEntry *entry = (DirEntry*)(fs->data + block_index * fs->sb->block_size);

    strcpy(entry[0].name,".");
    entry[0].id = 0;
    fs->current_dir->num_entries ++;

    strcpy(entry[1].name,"..");
    entry[1].id = 0; //La root punta a se stessa
    fs->current_dir->num_entries++;
    
    //Deve puntare a se stessa in questo caso
    //Nel caso di creazione cartella mkdir, il puntatore ".." deve puntare alla cartella padre
    //e per farlo devo fs->current_dir->block_ptrs[0] = block_index della cartella 
    //quindi mi conviene aggiungere direttamente l'id all'inode e usarlo per identificare la cartella padre

    // Sincronizza i dati su disco
    syncFS();
    return 0;
}


int loadFS(const char* filename) {
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


int createDirectory(const char* dirname) {
    //Controllo che non esiste già
    printf("Ciao1");
    if(findEntry(dirname) == 0){
          printf("È già presente una cartella con il nome %s",dirname);
          return -1;
    }

     printf("Ciao2");
    //Cerca inode libero
    Inode *new_inode = searchFreeInode();
    if(new_inode == NULL){
        printf("Tabella degli inode piena");
        return -1;
    }

    //Inizializza l'inode
    new_inode->is_dir=1;
    
    //Aggiunge le entry . e ..
    int block_index = searchFreeBlock();
    if(block_index < 0){
        printf("Spazio dei blocchi esaurito");
        return -1;
    }
    //Scrivo nel primo blocco della directory le entry . e ..
    new_inode->direct[0] = block_index;
    DirEntry *entry = (DirEntry*) (fs->data + block_index*fs->sb->block_size);
    
    strcpy(entry[0].name,".");
    entry[0].id = new_inode->id;
    new_inode->num_entries ++;

    strcpy(entry[1].name,"..");
    entry[1].id = fs->current_dir->id;
    new_inode->num_entries ++;

    //Aggiunge l'entry al padre
    addDirEntry(dirname,new_inode->id);

    syncFS();
    printf("\nDirectory %s salvata correttamente\n", dirname);
    return 0;
}


void printDirectory(){

}


void changeDirectory(const char *dirname) {
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
        //Per risparmiarmi la funzione per cerc
        DirEntry *entry = (DirEntry*)(fs->data + fs->current_dir->direct[0] * fs->sb->block_size);
        fs->current_dir = &fs->inodes[entry[1].id];
    } else {
        // directory normale
        int id = findEntry(dirname);
        if (id == -1) {
            printf("Directory non trovata: %s\n", dirname);
            return;
        }
        Inode *inode = &fs->inodes[id];
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