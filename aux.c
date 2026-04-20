#include "fs.h"


int search_free_inode(){
    for(int i=0; i < fs->sb->num_inodes; i++){
        if(fs->inode_table[i].used == 0){
            fs->inode_table[i].used=1;
            fs->sb->free_inodes--;
            return i;
        }
    }
    return -1; //Tabella degli inode piena
}

int search_free_block() {
    int num_blocks = fs->sb->num_blocks;

    for (int i = 0; i < num_blocks; i++) {
        int byte_index = i / 8;
        int bit_index  = i % 8;

        // Controlla se il bit è 0 (blocco libero)
        if ((fs->data_bitmap[byte_index] & (1 << bit_index)) == 0) {

            // Segna il blocco come occupato
            fs->data_bitmap[byte_index] |= (1 << bit_index);

            fs->sb->free_blocks--;

            return i;   // indice del blocco libero
        }
    }

    return -1; // nessun blocco libero
}