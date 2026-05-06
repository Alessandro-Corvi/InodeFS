#include "aux.h"

//---------Funzioni bitmpap---------
int bitmap_alloc() {
    for (int i = 0; i < fs->sb->num_blocks; i++) { 
        int byte = i / 8;
        int bit = 7 - (i % 8); //Se fosse i%8 il byte avrebbe l'ordine [7,6,5,4,3,2,1,0]
        if (!(fs->data_bitmap[byte] & (1 << bit))) { // bit a 0 = blocco libero
            fs->data_bitmap[byte] |= (1 << bit);
            fs->sb->free_blocks --;
            return i;
        }
    }
    printf("Nessun blocco libero\n");
    return -1;
}

void bitmap_free(int block_index){
    int byte = block_index /8;
    int bit = 7 - (block_index % 8);

    fs->data_bitmap[byte] &= ~(1 << bit);
    fs->sb->free_blocks++;
}
//---------------------------------------------------------



//------------------Funzioni per gli inode ----------------
/*Restituisce il blocco puntato dall'indice index*/
char* get_inode_block(Inode *inode, int index,bool clear_ref){
    char *block = NULL;
    if(index < NUM_DIRECT){
        //Se il puntatore non punta a nulla
        if(inode->direct[index] == NULL_PTR){return NULL;} 
        block = (char*)(fs->data + inode->direct[index] * fs->sb->block_size);
        
        //Remove: metto il puntatore a 0
        if(clear_ref){
            inode->direct[index] = NULL_PTR;
        }

    }else{
        //L'indiretto non è stato inizializzato
        if(inode->indirect == NULL_PTR) {return NULL;}

        //Prendi il blocco puntato dall'indiretto
        int *direct =(int*) (fs->data + inode->indirect * fs->sb->block_size);
        if(direct[index - NUM_DIRECT] == NULL_PTR){return NULL;}

        block = (char*)(fs->data + direct[index - NUM_DIRECT] * fs->sb->block_size);

        if(clear_ref){
            direct[index - NUM_DIRECT] = NULL_PTR;

            //Controlla se  i puntatori diretti puntati dall'indiretto
            //sono vuoti
            bool all_null = true;
            for(int i = 0; i < PTR_PER_BLOCK; i++){
                if(direct[i] != NULL_PTR){
                    all_null = false;
                }
            }

            //Se i puntori sono tutti a NULL_PTR elimino il blocco
            if(all_null){
                //Imposta il blocco a 0
                memset(direct, 0 ,fs->sb->block_size);
                //Rilascia il blocco
                bitmap_free(inode->indirect);
                //Imposta il puntatore a NULL
                inode->indirect = NULL_PTR;
            }
        }
        
    }

    return block;
}



/*Alloca il blocco nel puntatore index*/
int alloc_inode_block(Inode *inode, int index){
    if(index < NUM_DIRECT){
        int block_index = bitmap_alloc();
        if(block_index < 0){
            printf("Errore: memoria dati esaurita");
            return -1;
        }
        inode->direct[index] = block_index;

    }else{

        int block_index = bitmap_alloc();
            if(block_index < 0){
                printf("Errore: memoria dati esaurita");
                return -1;
        }
        //Se l'indice indiretto non è stato allocato
        if(inode->indirect == NULL_PTR){
            inode->indirect = block_index;
            int *direct_ptr = (int*) (fs->data + block_index * fs->sb->block_size);

            //Inizializzo i puntatori a NULL_PTR
            int ptr_per_block = fs->sb->block_size / sizeof(int);
            for(int i=0; i < ptr_per_block; i++){direct_ptr[i] = NULL_PTR;}

            block_index = bitmap_alloc();
            if(block_index < 0){
                printf("Errore: memoria dati esaurita"); 
                //Rilascia il blocco allocato per l'indirect
                bitmap_free(inode->indirect); 
                inode->indirect = NULL_PTR;
                return -1;
            }
           direct_ptr[index-NUM_DIRECT] = block_index; 
        }else{
            //Se l'indice indiretto è stato allocato, aggiungi l'indice
            int *direct_ptr =(int*) (fs->data + inode->indirect * fs->sb->block_size);
            direct_ptr[index-NUM_DIRECT] = block_index;
        }
    }
    return index;
}


//Restituisce il primo inode libero
Inode* search_free_inode(){
    for(int i = 0; i < fs->sb->num_inodes; i++){
        if(!(fs->inodes[i].used)){
            printf("Inode libero %d\n",i);
            fs->inodes[i].used = 1;
            fs->sb->free_inodes --;
            return &(fs->inodes[i]);
        }
    }
    return NULL;
}



//----------------Funzioni per le entry -------------------

//Cerca entry per nome nell'inode
DirEntry* find_entry(Inode *inode, const char* name){
     for(int index = 0; index < NUM_PTRS; index++){
        char *block = get_inode_block(inode,index,false);
        if(block == NULL){continue;}
        DirEntry *entry = (DirEntry*) block;
        for(int j=0; j< (int)(fs->sb->block_size/sizeof(DirEntry)); j++){
            if(strcmp(entry[j].name,name) == 0){
                return &entry[j];
            }
        }
    }
    return NULL;
}



//Funzione aggiunge l'entry (id,name) alla directory passata in ingresso
void add_dir_entry (Inode *inode, const char* name, int id){
    int free_ptr = NULL_PTR;
    for(int i=0; i< NUM_PTRS; i++){
        char *block = get_inode_block(inode,i,false);
        //Se il blocco non è stato allocato
        if(block == NULL){
            //Se ancora non è stato trovato il primo puntatore libero
            if(free_ptr == NULL_PTR){
                free_ptr = i;
            }
            continue;
        }
        DirEntry *entries = (DirEntry*) block;
        int entries_per_block = fs->sb->block_size / sizeof(DirEntry);
        for(int j=0; j < entries_per_block; j++){
            if(entries[j].name[0] == '\0'){

                //Aggiunge la nuova entry
                strcpy(entries[j].name, name);
                entries[j].id = id;
                
                //Aumenta le entry della directory
                inode->size += sizeof(DirEntry);
                return;
            }
        }
    }
    //Se non ho trovato spazio nei blocchi allocati ne alloco uno nuovo
    // nel primo puntatore che trovo
    if(free_ptr != NULL_PTR){
        int ptr_allocated = alloc_inode_block(inode, free_ptr);
        DirEntry *entries = (DirEntry*)get_inode_block(inode, ptr_allocated,false);
        
        //Aggiunge la nuova entry
        strcpy(entries[0].name, name);
        entries[0].id = id;
        
        //Aumenta le entry della directory
        inode->size += sizeof(DirEntry) ;
    }

}

/*Rimuove l'entry che un certo id nell'inode passato*/
int remove_dir_entry(Inode *inode, int id){
    for(int i = 0; i< NUM_PTRS; i++){
        char *block = get_inode_block(inode,i,false);
        if(block == NULL){continue;}
        DirEntry *entries = (DirEntry*)(block);
        int entries_per_block = fs->sb->block_size / sizeof(DirEntry);
        for(int j = 0; j < entries_per_block; j++){
            if(entries[j].id == id){
                memset((DirEntry*)&entries[j], 0 , sizeof(DirEntry));
                inode->size -= sizeof(DirEntry);
                return 0;
            }
        }
    }

    return -1;
}

//Funzione per sincronizzare direttamente con il disco
int syncFS() {
    if (fs == NULL) {
        printf("Errore: filesystem non inizializzato\n");
        return -1;
    }

    if (msync(fs->sb, fs->sb->fs_size, MS_SYNC) == -1) {
        perror("msync");
        return -1;
    }

    return 0;
}

