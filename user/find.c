#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#ifndef NULL
#define NULL 0
#endif

int VERBOSE = 0;

char *strrchr(const char *s, char c)
{
    for(const char *ptr = s + strlen(s) - 1; ptr >= s; ptr-- ){
        if(*ptr == c){
            return (char*)ptr;
        }
    }
    return NULL;
}

void pushd(char *buf, char *name)
{
    char *ptr = buf + strlen(buf);
    *ptr++ = '/';
    strcpy(ptr, name);
    *(ptr + strlen(name)) = '\0';
}

void popd(char *buf)
{
    char* ptr = strrchr(buf, '/');
    if(ptr == NULL){
        *buf = '\0';
    }
    else{
        *ptr = '\0';
    }
}

void find(char *path, char *name)
{
    
    if(VERBOSE){
        printf("find(%s, %s) called\n", path, name);
    }
    char buf[512];
    int fd;
    struct dirent de;
    struct stat st;

    if ((fd = open(path, O_RDONLY)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }

    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    if (st.type != T_DIR)
    {
        fprintf(2, "find: %s is not a directory\n", path);
        close(fd);
        return;
    }
    
    memset(buf, '\0', 512);
    strcpy(buf, path);

    while (read(fd, &de, sizeof(de)) == sizeof(de))
    {
        if(de.inum == 0){
            continue;
        }
        pushd(buf, de.name);
        if(VERBOSE){
            printf("find: checking <%s> in <%s>\n", de.name, buf);
        }
        if(stat(buf, &st)){
            fprintf(2, "find: cannot stat %s\n", buf);
            continue;
        }
        if(st.type == T_FILE){
            if(!strcmp(de.name, name)){
                printf("%s\n", buf);
            }
        }
        else if(st.type == T_DIR){
            if(strcmp(de.name, ".") && strcmp(de.name, "..")){
                find(buf, name);
            }
        }
        popd(buf);
        if(VERBOSE){
            printf("find: buffer poped: <%s>\n", buf);
        }
    }
}

int main(int argc, char *argv[])
{

    if (argc < 3)
    {
        fprintf(2, "Usage: find <path> <name>\n");
        exit(1);
    }

    if(!strcmp(argv[1], "-v")){
        printf("find: verbose mode on\n");
        VERBOSE = 1;
        find(argv[2], argv[3]);
    }
    else{
        find(argv[1], argv[2]);
    }

    exit(0);
}
