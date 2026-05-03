#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "fs.h"
#include "aux.h"

#define MAX_LINE    2048
#define MAX_TOKENS  256


void list(){
    printf("--------------\n");
    printf("-list: lista i comandi presenti ");
    printf("\n-format (filename) (size): formatta il filesystem  con taglia size");
    printf("\n-load (filename): carica il filesystem presente nella cartella");
    printf("\n-touch (filename): crea il file filename ");
    printf("\n-mkdir (dirname) : crea una cartella di nome dirname");
    printf("\n-cd (dirname): cambia il percorso attuale");
    printf("\n-cat (filename): stampa il contenuto del file filename sullo schermo");
    printf("\n-append (filename) (testo): aggiunge al file filename il testo");
    printf("\n-rmdir (dirname): rimuovo la cartella dirname");
    printf("\n-rmfile (filename); rimuove il file filename");
    printf("\n-close: fa l'unmount della memoria mappata e salva le modifiche\n");
}


void do_cmd(char* argv[MAX_TOKENS],int argc) {
    /*  HELP COMMANDS */
    if (strcmp(argv[0], "list") == 0) {
        list();
    }else if(strcmp(argv[0],"format")==0){
        if(argv[1]==NULL){
            printf("Errore: parametri in ingresso non validi\n");
            return;
        }
        if(argv[2]==NULL){
            printf("Errore: size %s non valida\n", argv[2]);
            return;
        }
        format_fs(argv[1],atoi(argv[2]));
    }else if(strcmp(argv[0],"load")==0){
        if(argv[1]==NULL){
            printf("Errore: filename non valido\n");
            return;
        }
        load_fs(argv[1]);
    }
    
    else if(strcmp(argv[0],"mkdir")==0){
        if(argv[1] == NULL){
            printf("Errore : nome non valido\n");
            return;
        }
        create_dir(argv[1]);
    }else if(strcmp(argv[0],"cd")==0){
        if(argv[1] == NULL){
            printf("Errore : nome non valido\n");
            return;
        }
        change_dir(argv[1]);
    }else if(strcmp(argv[0],"touch")==0){
        if(argv[1] == NULL || argc>2){
            printf("Parametri in ingresso non validi\n");
            return;
        }
        if(strchr(argv[1], '.') == NULL){
            printf("Errore: il nome del file deve contenere l'estensione '.'\n");
            return;
        }
        create_file(argv[1]);
    }else if(strcmp(argv[0],"cat")==0){
        if(argv[1]==NULL){
            printf("Nome del file non inserito");
            return;
        }
        
        if(strchr(argv[1], '.') == NULL){
            printf("Errore: il nome del file deve contenere l'estensione '.'\n");
            return;
        }

        read_file(argv[1]);
    }else if(strcmp(argv[0],"ls")==0){
        int inodes_mode = 0;
        if(argc == 2 && strcmp(argv[1],"-i")==0){
            inodes_mode = 1;
        }
        print_dir(inodes_mode); //Stampa solo la directory corrente
    }else if(strcmp(argv[0],"append")==0){
        if(argv[2] == NULL){
            printf("Non hai inserito il testo da scrivere nel file\n");
            return;
        }
        //Vedo se ha inserito un file senza estensione
        if(strchr(argv[1], '.') == NULL){
            printf("Errore: il nome del file deve contenere l'estensione '.'\n");
            return;
        }

        int total_len = 0;
        for(int i = 2; argv[i] != NULL; i++)
            total_len += strlen(argv[i]) + 1;  // +1 per spazio/terminatore

        char *buffer = malloc(total_len);
        buffer[0] = '\0';
        for(int i = 2; argv[i] != NULL; i++){
            if(i > 2) strcat(buffer, " ");
            strcat(buffer, argv[i]);
        }

        write_file(argv[1], buffer);
        free(buffer);
    }/*else if(strcmp(argv[0],"rmfile")==0){

    }*/else if(strcmp(argv[0],"rmdir")==0){
        if(argv[1] == NULL || (argc>2) ){
            printf("Parametri in ingresso non validi\n");
            return;
        }
        char *token = strtok(argv[1], ".");
        if(strcmp(token, argv[1])){
            printf("Errore: il nome della directory non può contenere '.'\n");
            return;
        }
        remove_dir(argv[1]);
    }
    

   
    /*  EXIT COMMAND */
    else if (strcmp(argv[0], "close") == 0) {
        _exit(0);
    }else{
        printf("unkwnown command %s\n", argv[0]);
    }
}


void deallocate_cmd(char* argv[MAX_TOKENS]) {
	while (*argv != NULL)
		free(*argv++);
}


char* dup_string(const char* in) {
    size_t n = strlen(in);
    char* out = malloc(n + 1);
    strcpy(out, in);
    return out;
}

void get_cmd_line(char* argv[MAX_TOKENS], int* argc) {
    char line[MAX_LINE];
    fgets(line, MAX_LINE, stdin);
    char* token = strtok(line, " \t\n");
    *argc = 0;
    while (*argc < MAX_TOKENS && token != NULL) {
        argv[(*argc)++] = dup_string(token);
        token = strtok(NULL, " \t\n");
    }
    argv[*argc] = NULL;
}

int do_shell(char *prompt){
    printf("list: lista i comandi disponibili\n");
    for (;;) {
        if(fs!= NULL){
            printf("%s",fs->current_path);
        }
        printf("%s",prompt);
        char* argv[MAX_TOKENS];
        int argc = 0;
        get_cmd_line(argv,&argc);
        if (argv[0] == NULL) continue;
        if (strcmp(argv[0], "quit") == 0) break;
        do_cmd(argv, argc);
	    deallocate_cmd(argv);
    }
    return EXIT_SUCCESS;
}



int main(){
    return do_shell(">>");
}