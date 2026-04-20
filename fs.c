#include "fs.h"
#include "aux.h"

FileSystem *fs =NULL;


int format_fs(const char *filename, int size){
    int fd = open(filename, O_CREAT|O_RDWR , 0666);
    if(fd == -1){
        printf("Errore nella creazione del file");
        return -1;
    }

    if(ftruncate(fd,size) == -1){
        printf("Errore nel ridimensionamento del filesystem");
        return -1;
    }

    void *ptr = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED,fd, 0);
    if(ptr == MAP_FAILED){
        printf("Errore nella mappatura del file");
        close(fd);
        return -1;
    }

    close(fd);

    //Inizializzo la struttura globale
    fs = (FileSystem*) malloc(sizeof(FileSystem));
    memset(fs,0, sizeof(FileSystem));

    //Inizializzo superblocco
    fs->sb = (Superblock*) ptr;
    fs->sb->fs_size = size;
    fs->sb->block_size =  4096;
    fs->sb->num_inodes = 256;
    fs->sb->num_blocks = fs->sb->fs_size / fs->sb->block_size;
    fs->sb->free_inodes = 256;
    fs->sb->free_blocks = fs->sb->num_blocks;


    int bitmap_size = fs->sb->num_blocks / 8;
    int inode_table_size = fs->sb->num_inodes * sizeof(Inode);

    fs->data_bitmap = (char*)ptr + sizeof(Superblock);
    fs->inode_table = (Inode*)(fs->data_bitmap + bitmap_size);
    fs->data = (char*)fs->inode_table +inode_table_size;


    //Inizializzo la root
    fs->current_dir = (Inode*) &(fs->inode_table[0]); //Devo aggiornare la fs->sb->free_inode
    fs->current_dir->is_dir = 1;
    fs->current_dir->parent_id = 0;
    fs->current_dir->size = 0;
    fs->sb->free_inodes --;

    int block_idx = search_free_block(); //Devo aggiornare la fs->sb->free_block
    if(block_idx == -1){
        printf("Spazio dei data block esaurito");
        return -1;
    }

    fs->current_dir->direct[0] = block_idx;

    DirEntry *entries = (DirEntry*) (fs->data + block_idx * fs->sb->block_size);
    
    //Aggiungi entry . 
    strcpy(entries[0].name,".");
    entries[0].id = 0;


    //Aggiungi entry ..
    strcpy(entries[1].name,"..");
    entries[1].id = 0;


    msync(fs->sb, fs->sb->fs_size, MS_SYNC);
    return 0;
}




