#define FUSE_USE_VERSION 31
#define MAX_USERS 1000

#include <fuse3/fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static int vfs_pid = -1;

static struct passwd *users[MAX_USERS];
static int user_count = 0;

// Récupère la liste des utilisateurs
int get_users_list() {
    struct passwd *pw;
    setpwent();
    user_count = 0;

    while ((pw = getpwent()) != NULL && user_count < MAX_USERS) {
        if (pw->pw_uid >= 1000) {
            users[user_count++] = pw;
        }
    }

    endpwent();
    return user_count;
}

// Libère la liste
void free_users_list() {
    user_count = 0;
}

// Récupère les attributs d’un fichier ou dossier
static int users_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi) {
    (void)fi;
    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    for (int i = 0; i < user_count; i++) {
        char user_path[1024];
        snprintf(user_path, sizeof(user_path), "/%s", users[i]->pw_name);

        if (strcmp(path, user_path) == 0) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
    }

    return -ENOENT;
}

// Lire le contenu du dossier racine
static int users_readdir(
    const char *path,
    void *buf,
    fuse_fill_dir_t filler,
    off_t offset,
    struct fuse_file_info *fi,
    enum fuse_readdir_flags flags
) {
    (void)offset;
    (void)fi;
    (void)flags;

    if (strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    for (int i = 0; i < user_count; i++) {
        filler(buf, users[i]->pw_name, NULL, 0, 0);
    }

    return 0;
}

// Création d’un utilisateur via mkdir
static int users_mkdir(const char *path, mode_t mode) {
    (void)mode;

    const char *username = path + 1; // retirer le "/"

    int pid = fork();
    if (pid == 0) {
        execl("/usr/sbin/useradd",
              "useradd",
              "-m",
              "-s",
              "/bin/bash",
              username,
              NULL);
        exit(1);
    }
    wait(NULL);

    get_users_list();
    return 0;
}

// Définition des opérations FUSE
static struct fuse_operations users_oper = {
    .getattr = users_getattr,
    .readdir = users_readdir,
    .mkdir = users_mkdir,
};

// Démarrage de la VFS
int start_users_vfs(const char *mount_point) {
    int pid = fork();
    if (pid == 0) {
        char *fuse_argv[] = {
            "users_vfs",
            "-f",
            "-s",
            (char*)mount_point,
            NULL
        };

        if (get_users_list() <= 0) {
            exit(1);
        }

        int ret = fuse_main(4, fuse_argv, &users_oper, NULL);
        free_users_list();
        exit(ret);
    } else {
        vfs_pid = pid;
        return 0;
    }
}

// Arrêt de la VFS
void stop_users_vfs() {
    if (vfs_pid != -1) {
        kill(vfs_pid, SIGTERM);
        waitpid(vfs_pid, NULL, 0);
        vfs_pid = -1;
    }
}
