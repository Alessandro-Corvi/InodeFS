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
   

    // Inizializza il file system 
    int block_size = 512;
    int num_inodes = 128;
    int overhead = sizeof(Superblock) + sizeof(Inode) *num_inodes;

    int num_blocks = (int)((fs_size - overhead) * 8) / (8 * block_size + 1);
    fs->sb = (Superblock*)ptr;
    fs->sb->block_size  = block_size;
    fs->sb->num_inodes  = num_inodes;
    fs->sb->num_blocks  = num_blocks;
    fs->sb->free_blocks = num_blocks;
    fs->sb->free_inodes = num_inodes;

    fs->data_bitmap = ptr + sizeof(Superblock);
    fs->inodes = (Inode*)(fs->data_bitmap + fs->sb->num_blocks / 8);
    fs->data = (char*)(fs->inodes + fs->sb->num_inodes);

    //Inizializza l'inode table
    for(int i=0; i < fs->sb->num_inodes; i++){
        Inode *inode = (Inode*) &(fs->inodes[i]);
        inode->id=i;
        //Imposto tutti i puntatori a -1
        memset(inode->direct, NULL_PTR, sizeof(inode->direct));
        inode->indirect = NULL_PTR;
    }

    //Inizializza la  Root
    Inode *inode = search_free_inode();
    inode->is_dir = 1;
    inode->size = 0;

    fs->current_dir = inode;
    fs->current_path[0] = '/';

    // Sincronizza i dati su disco
    return syncFS();
}

/*Funzione per ricaricare il filesystem presente nella cartella */
int load_fs(const char* filename) {
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        printf("Errore: impossibile aprire il file %s\n", filename);
        return -1;
    }

    //Ottenie la lunghezza del file
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
    fs->current_dir = &fs->inodes[0]; //Imposto la directory corrente 
    fs->current_path[0] = '/';  
    return 0;
}

/*Crea una nuova directory nella current_dir*/
int create_dir(const char* dirname){
   if(fs==NULL){
        printf("Filesystem non ancora formattato\n");
        return -1;
    }

    //Tutti i puntatori pieni
    if(fs->current_dir->size == MAX_SIZE){
        printf("Errore la tabella corrente ha tutte le entry piene\n");
        return -1;
    }

    //Controlla che la directory che non esiste già
    if(find_entry(fs->current_dir,dirname) != NULL){
          printf("È già presente una cartella con il nome %s\n",dirname);
          return -1;
    }

    //Cerca inode libero
    Inode *new_inode = search_free_inode();
    if(new_inode == NULL){
        printf("Tabella degli inode piena\n");
        return -1;
    }

    //Inizializza l'inode
    new_inode->is_dir = 1;
    new_inode->size = 0;


    //Aggiunge le entry . e ..
    add_dir_entry(new_inode,".",new_inode->id);
    add_dir_entry(new_inode,"..",fs->current_dir->id);

    //Aggiunge l'entry al padre
    add_dir_entry(fs->current_dir,dirname,new_inode->id);

    printf("Directory %s salvata correttamente\n", dirname);
    return syncFS();
}


//Stampa la directory corrrente
void print_dir(bool inodes_mode){
    if(fs==NULL){
        printf("Filesystem non ancora formattato\n");
        return;
    }
    if(inodes_mode){
        printf("Inode-Name\n");
    }

    int total_entries = fs->current_dir->size / sizeof(DirEntry);  
    int read_entries = 0;
    for (int i = 0; i < NUM_PTRS && read_entries < total_entries; i++){
        
        char *block = (char*)get_inode_block(fs->current_dir,i,false);
        if(block==NULL){continue;}
        
        DirEntry *entries = (DirEntry*) block;
        int entries_per_block = fs->sb->block_size / sizeof(DirEntry);
        for(int j = 0; j < entries_per_block; j++){
            if(entries[j].name[0] == '\0'){continue;}

            if(inodes_mode){
                if(entries[j].name[0] == '\0'){continue;}
                printf("%d  %s\n",entries[j].id,entries[j].name);
            }
            else{
                if(entries[j].name[0] == '\0'){continue;}
                printf("%s\n",entries[j].name);
            }
            read_entries ++;
        }

    }
}

//Cambia directory
void change_dir(const char *dirname) {
    if(fs==NULL){
        printf("Filesystem non ancora formattato\n");
        return;
    }

    if (strcmp(".", dirname) == 0) {
        return;
    } else if (strcmp("..", dirname) == 0) {
        //Si trova nella root
        if (strcmp(fs->current_path, "/") == 0) {
            return; 
        }

        //Trova ..
        DirEntry *parent = find_entry(fs->current_dir, "..");
        if(parent == NULL){
            printf("Errore: directory padre non trovata\n");
            return;
        }

        fs->current_dir = &fs->inodes[parent->id];

        //Aggiorna il path: tronca all'ultimo /
        char *last_slash = strrchr(fs->current_path, '/');
        if (last_slash == fs->current_path) {
            *(last_slash + 1) = '\0'; // torna a "/"
        } else {
            *last_slash = '\0'; // rimuove ultimo componente
        }
        
    } else {
        // directory normale
        DirEntry *entry= find_entry(fs->current_dir,dirname);
        if (entry == NULL) {
            printf("Directory non trovata: %s\n", dirname);
            return;
        }
        Inode *inode = &(fs->inodes[entry->id]);
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
/*Rimossa la directory*/
int remove_dir(const char* dirname, bool forced){
    if(fs==NULL){
        printf("Filesystem non ancora formattato\n");
        return -1;
    }

    DirEntry *entry = find_entry(fs->current_dir,dirname);
    if(entry == NULL){
        printf("Non è presente la cartella con nome %s\n", dirname);
        return -1;
    }

    Inode *inode = (Inode*) &(fs->inodes[entry->id]);

    if(!forced && (inode->size > (int)(2*sizeof(DirEntry)))){
        printf("La cartella non è vuota\n");
        return -1;
    }

    
    if(forced){
        Inode *saved_dir = fs->current_dir;
        fs->current_dir = inode; 

        for(int i = 0; i < NUM_PTRS; i++){
            char *block = get_inode_block(inode, i, false); 
            if(block == NULL) continue;

            DirEntry *entries = (DirEntry*)block;
            int num_entries = fs->sb->block_size / sizeof(DirEntry);

            for(int j = 0; j < num_entries; j++){
                DirEntry *entry = &entries[j];
                if(entry->name[0] == '\0') continue;
                if(strcmp(entry->name, ".") == 0) continue;    
                if(strcmp(entry->name, "..") == 0) continue;   

                Inode *child = &fs->inodes[entry->id];
                if(child->is_dir){
                    if(remove_dir(entry->name, forced) == -1) {return -1;}

                }else{
                    if(remove_file(entry->name) == -1) return -1;
                }
            }
        }

        fs->current_dir = saved_dir; 
    }

    //Rimuovo le entry . e ..
    DirEntry *entries = (DirEntry*)(fs->data  + inode->direct[0] * fs->sb->block_size);
    memset(entries,0,2*sizeof(DirEntry));

    //Rimuovi la entry dal padre
    remove_dir_entry(fs->current_dir, inode->id);

    //Reset inode
    inode->used = 0;
    inode->is_dir = 0;
    inode->size = 0;

    printf("Sincronizza subito con il disco\n");
    return syncFS();
}



/*Funzione per creare un file*/
int create_file(const char* filename){
    if(fs==NULL){
        printf("Filesystem non ancora formattato\n");
        return -1;
    }

    //Controlla se ci sono tutte le direntry occupate
    if(fs->current_dir->size == MAX_SIZE){
        printf("Directory attuale piena.");
        return -1;
    }
  
    if(find_entry(fs->current_dir,filename) != NULL){
        printf("File con nome %s già presente ",filename);
        return -1;
    }

    //Cerca un inode libero per il file
    Inode *new_inode = search_free_inode();
    if(new_inode == NULL){
        printf("Tabella degli inode piena");
        return -1;
    }

    new_inode->is_dir = 0;
    new_inode->used = 1;
    new_inode->size=0;

    //Aggiunge la entry alla directory padre
    add_dir_entry(fs->current_dir,filename,new_inode->id);

    //Apporta le modifiche al disco
    printf("File creato con successo!!\n");
    return syncFS();
}

/*
ReadFile (filename) : {
    //1.Cerco l'inode corrispondente
    //2.Vedo quanti blocchi devo leggere (arrotondo per eccesso)
    //3. Leggo un blocco alla volta con get_inode_block()
    //
}
    */
/*Funzione per leggere un file*/
int read_file(const char* filename){
    //Il filesystem non è ancora formattato
    if(fs==NULL){
        printf("Filesystem non ancora formattato\n");
        return -1;
    }

    DirEntry *entry = find_entry(fs->current_dir,filename);
    if(entry == NULL){
        printf("Il file non è presente all'interno della cartella\n");
        return -1;
    }

    Inode *inode = (Inode*)&(fs->inodes[entry->id]);
    if(inode->size == 0){
        printf("Il file è vuoto\n");
        return -1;
    }

    int remaining  = inode->size;
    for(int i = 0; i < NUM_PTRS && remaining > 0; i++){
        printf("ptr %d\n",i);
        char *block = get_inode_block(inode, i, false);
        //Da quel punto in poi il file è finito
        if(block==NULL){break;}

        int to_read = fs->sb->block_size;
        if(remaining < fs->sb->block_size){
            to_read = remaining;
        }
        //Scrive in stdout to_read byte
        fwrite(block, 1, to_read, stdout);     
        remaining -= to_read;  
    }
    printf("\n");
    return 0;
}


int write_file(const char* filename, const char *data){
    if(fs==NULL){
        printf("Filesystem non ancora formattato\n");
        return -1;
    }
    
    //Vedo se è presente il file all'interno della cartella corrente
    DirEntry *entry = find_entry(fs->current_dir,filename);
    if(entry ==NULL){
        printf("Non è presente il file %s",filename);
        return -1;
    }

    Inode *inode = (Inode*) &(fs->inodes[entry->id]);

    // Prepara la stringa da scrivere con separatore se necessario
    char *to_write;
    if (inode->size > 0) {
        // Alloca spazio per " " + data + '\0'
        to_write = malloc(strlen(data) + 2);
        to_write[0] = ' ';
        strcpy(to_write + 1, data);
    } else {
        to_write = (char*)data;
    }

    int data_len = strlen(to_write);


    int block_needed = (int)ceil((double)strlen(to_write) / fs->sb->block_size);
    if(inode->size >= MAX_SIZE ){
        printf("Errore: il file ha raggiunto la dimensione massima.");
        return -1;
    }

    if(inode->size + (block_needed * fs->sb->block_size) > MAX_SIZE){
        printf("Errrore: impossibile scrivere, dimensione massima del file superata");
        return -1;
    }

    //Blocco da cui abbiamo inziato a scrivere
    int current_block = (int) ceil((double)inode->size / fs->sb->block_size);
    int total_blocks = current_block + block_needed;

    //Alloco i blocchi necessari
    for(int i=current_block; i < total_blocks; i++){
        if(alloc_inode_block(inode,i) < 0){
            printf("Errore nessun blocco libero disponibile");
            return -1;
        }
        printf("Allocato %d\n",i);
    }  

    int written = 0;
    int offset = inode->size;
    while(written < data_len){
        int block_index = (offset + written) / fs->sb->block_size;
        int block_offset = (offset + written) %fs->sb->block_size;

        char *block = get_inode_block(inode, block_index, false);
        if(block==NULL){
            printf("Errore nel prendere il blocco corrispondente\n");
            return -1;
        }

        int byte_to_write = fs->sb->block_size - block_offset;
        if(byte_to_write > data_len - written){
            byte_to_write = data_len - written;
        }

        memcpy(block + block_offset, (char*) to_write + written , byte_to_write);
        written += byte_to_write;
        inode->size += byte_to_write;
        
    }
    
    return syncFS();
}

int remove_file(const char* filename){
    //Filesystem non formattato
    if(fs == NULL){
        printf("Filesystem non ancora formattato\n");
        return -1;
    }

    //Cerca l'entry corrispondente
    DirEntry *entry = find_entry(fs->current_dir, filename);
    if(entry == NULL){
        printf("File non presente\n");
        return -1;
    }

    //Vai a prendere l'inode corrispondente
    Inode *inode = (Inode*)&(fs->inodes[entry->id]);

    //Puntatore al puntatore dell'inode che deve essere settato a 0
    int ptrs_to_remove = (ceil)((double)inode->size / fs->sb->block_size);

    for(int i = 0; i < ptrs_to_remove; i++){
        char *block = get_inode_block(inode, i, true);
        
        if(block == NULL){break;} 

        //Azzera i dati del blocco
        memset(block, 0, fs->sb->block_size);

        //Libera il blocco 
        bitmap_free(block);

    }

    //Aggiorna la directory padre
    remove_dir_entry(fs->current_dir, inode->id);

    //Libero l'inode 
    inode->used = 0;

    return syncFS();
}

/*
RemoveFile(filename): uso la funzione per cercare 
    findEntry
    //2.taglia/fs->block_size
    //3.elimino tutti i blocchi memset(block,0,fs->block_size) (Fatto)
    //4. imposto il valore 0 nella bitmap per i blocchi liberi (Fatto, ma devo fare la bitmap)
    //4.elimino anche i riferimenti ptr[i] = 0 
    //
    //5.elimino la entry dalla directory padre
    //6.Apporto le modifiche al disco
    */