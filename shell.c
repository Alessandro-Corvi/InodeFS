#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

#include "fs.h"

#define MAX_LINE    2048
#define MAX_TOKENS  256
#define MAX_PATH 128

char current_path[MAX_PATH];


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
            printf("Errore: parametri in ingresso non validi");
            return;
        }
        if(strcmp(argv[2],"0")==0){
            printf("Errore: size %s non valida", argv[2]);
            return;
        }
        if(argc == 4){return;}
        format_fs(argv[1],atoi(argv[2]));
    }
    /*
    else if(strcmp(argv[0],"mkdir")==0){

    }else if(strcmp(argv[0],"cd")==0){

    }else if(strcmp(argv[0],"touch")==0){

    }else if(strcmp(argv[0],"cat")==0){

    }else if(strcmp(argv[0],"ls")==0){

    }else if(strcmp(argv[0],"append")==0){

    }else if(strcmp(argv[0],"rmfile")==0){

    }else if(strcmp(argv[0],"rmdir")==0){

    }
    */

   
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
        printf("%s%s", current_path,prompt);
        char* argv[MAX_TOKENS];
        int argc = 0;
        get_cmd_line(argv,&argc);
        if (argv[0] == NULL) continue;
        if (strcmp(argv[0], "quit") == 0) break;
        do_cmd(argv,  argc);
	    deallocate_cmd(argv);
    }
    return EXIT_SUCCESS;
}



int main(){
    return do_shell(">>");
}