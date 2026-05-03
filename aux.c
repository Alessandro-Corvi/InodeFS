#include "aux.h"

char* get_inode_block(Inode *inode,int index){
    char *block = NULL;
    if(index < 12){
        if(inode->direct[index] == 0){return NULL;}
        block = fs->data + inode->direct[index] * fs->sb->block_size;
    }else{
        if(inode->indirect == 0) {return NULL;}
        int *direct =(int*) (fs->data + inode->indirect * fs->sb->block_size);
        if(direct[index-12]==0){return NULL;}
        block = fs->data + direct[index-12] + fs->sb->block_size;
    }
    return block;
}


int alloc_inode_block(Inode *inode, int index){
    int block_index = search_free_block();
    if(block_index < 0){printf("Errore: memoria dati esaurita");return -1;}

    if(index<12){
        inode->direct[index] = block_index;
    }else{
        //Se l'indice indiretto non ha un blocco
        if(index == 12){
            inode->indirect = block_index;
            int *direct_ptr = (int*) (fs->data + block_index * fs->sb->block_size);
            int data_block = search_free_block();
            if(data_block < 0){
                printf("Errore: memoria dati esaurita"); 
                return -1;
           }
           direct_ptr[0] = data_block; 
        }else{
            //Se l'indice indiretto ha un blocco allocato cerco direttamente il puntatore
            int *direct_ptr =(int*) (fs->data + inode->indirect * fs->sb->block_size);
            direct_ptr[index-NUM_DIRECT] = block_index;
        }
    }
    return index;
}



int search_free_block() {
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

DirEntry* search_free_entry(Inode *inode) {
    for(int i = 0; i < NUM_PTRS; i++) {
        char *block = get_inode_block(inode,i);
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


Inode* search_free_inode(){
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

DirEntry* findEntry(Inode *inode,const char* dirname){
    for(int index = 0; index < NUM_PTRS; index++){
        char *block = get_inode_block(inode,index);
        if(block == NULL){continue;}
        DirEntry *entry = (DirEntry*) block;
        for(int j=0; j< (int)(fs->sb->block_size/sizeof(DirEntry)); j++){
            if(strcmp(entry[j].name,dirname) == 0){
                return &entry[j];
            }
        }
    }
    return NULL;
}


void add_dir_entry(Inode *inode, const char* name, int id){
    int free_ptr = NULL_PTR;
    for(int i=0; i< NUM_PTRS; i++){
        char *block = get_inode_block(inode,i);
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
            if(entries[j].name[0] == '\0'){
                strcpy(entries[j].name, name);
                entries[j].id = id;
                inode->num_entries ++;
                return;
            }
        }
    }
    //Se non ho trovato entries libere nei blocchi allocati
    // ne alloco uno nuovo, free_ptr sarà
    int ptr_allocated = alloc_inode_block(inode, free_ptr);
    DirEntry *entries = (DirEntry*)get_inode_block(inode, ptr_allocated);
    
    //Aggiungo la nuova entry
    strcpy(entries[0].name, name);
    entries[0].id = id;
    
    //Aumento le entry della directory
    inode->num_entries ++ ;
}




//Rimuove la entry dall'inode passato
int remove_dir_entry(Inode *inode, int id){
    for(int i=0; i<NUM_PTRS; i++){
        DirEntry *entries = (DirEntry*)(get_inode_block(fs->current_dir,i));
        for(int j=0 ; j<ENTRIES_PER_BLOCK; j++){
            if(entries[j].id ==id){
                memset((DirEntry*)&entries[j],0,sizeof(DirEntry));
                inode->num_entries--;
                return 0;
            }
        }
    }
    return -1;
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