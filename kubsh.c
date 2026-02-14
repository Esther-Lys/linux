#define FUSE_USE_VERSION 31
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "vfs1.h"

#define HISTORY_FILE ".kubsh_history"

sig_atomic_t signal_received = 0;

// Handler pour SIGHUP
void sig_handler(int signum) {
    if (signum == SIGHUP) {
        printf("Configuration reloaded\n");
    }
    signal_received = signum;
}

// Fonction pour exécuter un binaire système
void fork_exec(char *full_path, char **argv) {
    int pid = fork();
    if (pid == 0) {
        execv(full_path, argv);
        perror("exec");
        exit(1);
    } else {
        wait(NULL);
    }
}

// Vérifie si le chemin est exécutable
int is_executable(const char *path) {
    return access(path, X_OK) == 0;
}

// Recherche un binaire dans le PATH
char *find_in_path(const char *command) {
    char *path_env = getenv("PATH");
    if (!path_env) return NULL;

    char *paths = strdup(path_env);
    char *dir = strtok(paths, ":");
    while (dir) {
        char full[512];
        snprintf(full, sizeof(full), "%s/%s", dir, command);
        if (is_executable(full)) {
            free(paths);
            return strdup(full);
        }
        dir = strtok(NULL, ":");
    }
    free(paths);
    return NULL;
}

// Fonction pour afficher une variable d'environnement
void print_env_var(const char *input) {
    if (input == NULL || strlen(input) == 0) {
        printf("Usage: \\e $VARNAME\n");
        return;
    }
    char buffer[256];
    strncp(buffer, input, sizeof(buffer)-1);
    buffer[sizeof(buffer) -1] ='\0';

    char *var_name =buffer;
    while(*var_name == ' ' || *var_name== '\t'){
      var_name++;
    }
    // Ignorer le $ si présent
    if (var_name[0] == '$')
        var_name++;

    char *end =var_name + strlen(var_name)-1;
    while(end >var_name && *end ==' '){
      *end= '\0';
      end--;
    }


    const char *value = getenv(var_name);
    if (value == NULL) {
        printf("Environmental variable '%s' not found.\n", var_name);
        return;
    }

    printf("%s =\n", var_name);
    
    // Si la variable contient ':', on l'affiche ligne par ligne
    if (strchr(value, ':')) {
        char *value_copy = strdup(value);
        if(value_copy){
            char *token = strtok(value_copy, ":");
            while (token) {
               printf(" - %s\n", token);
               token = strtok(NULL, ":");
            }
            free(value_copy);
         }
   }
    else{
      printf(" %s\n", value);
    }
}

// Commande echo
void echo_command(char *input) {
    char *text = input + 5; // Skip "echo "
    // Enlever les guillemets si présents
    if (text[0] == '"' && text[strlen(text)-1] == '"') {
        text[strlen(text)-1] = '\0';
        text++;
    }
    printf("%s\n", text);
}

// Commande debug
void debug(char *line) {
    printf("%s\n", line);
}

// Liste les partitions (commande \l)
void list_partitions(const char *device) {
    char command[512];
    
    if (device && strlen(device) > 0) {
        snprintf(command, sizeof(command), 
                 "sudo fdisk -l %s 2>/dev/null || sudo lsblk %s", 
                 device, device);
    } else {
        snprintf(command, sizeof(command), "sudo lsblk");
    }
    
    system(command);
}

// Exécute une commande système
void execute_command(char *input) {
    char *argv[64];
    int argc = 0;
    
    char *token = strtok(input, " ");
    while (token && argc < 63) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;
    
    if (argv[0] == NULL) {
        return;
    }
    
    // Commande interne 'cd'
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1]) {
            if (chdir(argv[1]) != 0) {
                perror("cd");
            }
        } else {
            chdir(getenv("HOME"));
        }
        return;
    }
    
    // Chercher la commande dans le PATH
    char *cmd_path = find_in_path(argv[0]);
    
    if (cmd_path) {
        fork_exec(cmd_path, argv);
        free(cmd_path);
    } else if (access(argv[0], X_OK) == 0) {
        // Commande avec chemin absolu ou relatif
        fork_exec(argv[0], argv);
    } else {
        printf("%s: command not found\n", argv[0]);
    }
}
//lisst_disq_usage
void list_disk_usage(){
    int ret= system("-df -h 2>/dev/null");
    if (ret !=0){
       printf("(NOT MOUNTED FILESYSTEM OR df ERROR)\n");
    }

}

// Programme principal
int main() {
    // Configuration des signaux
    signal(SIGHUP, sig_handler);
    signal(SIGINT, SIG_IGN); // Ignorer Ctrl+C dans le shell
    
    // Historique
    char *home = getenv("HOME");
    char history_path[512];
    snprintf(history_path, sizeof(history_path), "%s/%s", home ? home : ".", HISTORY_FILE);
    
    FILE *test = fopen(history_path, "r");
    if (test) {
        fclose(test);
        read_history(history_path);
    }
    
    // Démarrage de la VFS
    printf("Démarrage de la VFS...\n");
    if (start_users_vfs("users") == 0) {
        printf("VFS démarrée sur /users\n");
    } else {
        printf("Erreur: Impossible de démarrer la VFS\n");
    }
    
    printf("Kubsh started. Type \\q to quit.\n");
    printf("Commandes disponibles:\n");
    printf("  \\e $VAR  - Afficher une variable d'environnement\n");
    printf("  \\l [dev] - Lister les partitions\n");
    printf("  echo     - Afficher du texte\n");
    printf("  debug    - Afficher la ligne de commande\n");
    printf("  \\q       - Quitter\n");
    
    char *input;
    
    // Boucle principale
    while (true) {
        input = readline("$> ");
        
        if (signal_received) {
            signal_received = 0;
            if (input) free(input);
            continue;
        }
        
        if (!input) {
            printf("\n");
            break;
        }
        
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        add_history(input);
        
        // Traitement des commandes spéciales
        if (!strcmp(input, "\\q")) {
            free(input);
            break;
        }
        else if (strncmp(input, "debug ", 6) == 0) {
            debug(input + 6);
        }
        else if (strncmp(input, "echo ", 5) == 0) {
            echo_command(input);
        }
        else if (strncmp(input, "\\e ", 3) == 0) {
            print_env_var(input + 3);
        }
        else if (strncmp(input, "\\l", 2) == 0) {
            char *device = input + 2;
            while (*device == ' ') device++;
            list_partitions(device);
        }
        else if (strncmp(input, "\\ldf", 4)==0){
            char *option = input +4;
            while(*option == ' ') option++;
            list_disk_usage(option);
        }
        else {
            execute_command(input);
        }

        free(input);
    }

    // Sauvegarde de l'historique et nettoyage
    write_history(history_path);
    stop_users_vfs();

    printf("Goodbye!\n");
    return 0;
}
