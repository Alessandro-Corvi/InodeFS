#include "aux.h"

char* getBlockAt(int index){
    char *block = NULL;
    if(index < 12){
        if(fs->current_dir->direct[index] == 0){return NULL;}
        block = fs->data + fs->current_dir->direct[index] * fs->sb->block_size;
    }else{
        int *direct =(int*) (fs->data + fs->current_dir->indirect * fs->sb->block_size);
        if(direct[index]==0){return NULL;}
        block = fs->data + direct[index] + fs->sb->block_size;
    }
    return block;
}


int allocBlockAt(int index){
    int block_index = searchFreeBlock();
    if(block_index < -1){printf("Errore: memoria dati esaurita");return -1;}

    if(index<12){
        fs->current_dir->direct[index] = block_index;
    }else{
        //Se l'indice indiretto non ha un blocco
        if(index == 12){
            fs->current_dir->indirect = block_index;
            int *direct_ptr = (int*) (fs->data + block_index * fs->sb->block_size);
            int block_index = searchFreeBlock();
            if(block_index < 0){
                printf("Errore: memoria dati esaurita"); 
                return -1;
           }
           direct_ptr[0] = block_index; 
        }else{
            //Se l'indice indiretto ha un blocco allocato cerco direttamente il puntatore
            int *direct_ptr =(int*) (fs->data + fs->current_dir->indirect * fs->sb->block_size);
            direct_ptr[index] = block_index;
        }
    }
    return index;
}



int searchFreeBlock() {
    for (int i = 1; i < fs->sb->num_blocks; i++) { // parte da 1 perché il blocco 0 è riservato
        int byte = i / 8;
        int bit = 7 - (i % 8);
        if (!(fs->data_bitmap[byte] & (1 << bit))) { // bit a 0 = blocco libero
            fs->data_bitmap[byte] |= (1 << bit);
            return i;
        }
    }
    printf("Nessun blocco libero\n");
    return -1;
}

DirEntry* searchFreeEntry() {
    for(int i = 0; i < NUM_PTRS; i++) {
        char *block = getBlockAt(i);
        if (!block) continue;

        DirEntry *entry = (DirEntry*) block;
        for(int j = 0; j < (int)(fs->sb->block_size / sizeof(DirEntry)); j++) {
            if(entry[j].id == 0) {
                return &entry[j];
            }
        }
    }
    return NULL;
}


Inode* searchFreeInode(){
    for(int i=0; i < fs->sb->free_inodes; i++){
        Inode *inode = (Inode*) &(fs->inodes[i]);
        if(!(inode->used)){
            inode->used = 1;
            fs->sb->free_inodes--;
            return inode;
        }  
    }
    return NULL;
}

int findEntry(const char* dirname){
    for(int index = 0; index < NUM_PTRS; index++){
        char *block = getBlockAt(index);
        if(block == NULL){continue;}
        DirEntry *entry = (DirEntry*) block;
        for(int j=0; j< (int)(fs->sb->block_size/sizeof(DirEntry)); j++){
            if(strcmp(entry[j].name,dirname) == 0){
                return 0;
            }
        }
    }
    return -1;
}



//Aggiunge la entry all'inode 
int addDirEntry(const char* name, int id) {
    if(fs->current_dir->num_entries == MAX_ENTRIES){
        printf("Cartella piena");
        return -1;
    }

    int free_ptr = NULL_PTR;
    for(int i=0; i< NUM_PTRS; i++){
        char *block = getBlockAt(i);
        //Se il blocco non è stato allocato
        if(block==NULL){
            //Se ancora non è stato trovato il primo puntatore libero
            if(free_ptr == NULL_PTR){
                free_ptr = i;
            }
            continue;
        }
        DirEntry *entries = (DirEntry*) block;
        for(int j=0; j < ENTRIES_PER_BLOCK; j++){
            if(entries[j].id==0){
                strcpy(entries[j].name,name);
                entries[j].id = id;
                fs->current_dir->num_entries ++;
                return 0;
            }
        }
    }
    //Se non ho trovato entries libere nei blocchi allocati
    // ne alloco uno nuovo
    int ptr_allocated = allocBlockAt(free_ptr);
    DirEntry *entries = (DirEntry*)getBlockAt(ptr_allocated);
    
    //Aggiungo la nuova entry
    strcpy(entries[0].name, name);
    entries[0].id = id;
    
    //Aumento le entry della dir corrente
    fs->current_dir->num_entries ++ ;
    
    //Sincronizzo con il disco
    syncFS();
    return 0;
}


//Funzione per sincronizzare direttamente con il disco
int syncFS() {
    if (fs == NULL || fs->sb == NULL) {
        printf("Errore: filesystem non inizializzato\n");
        return -1;
    }

    if (msync(fs->sb, fs->sb->fs_size, MS_SYNC) == -1) {
        perror("msync");
        return -1;
    }

    return 0;
}