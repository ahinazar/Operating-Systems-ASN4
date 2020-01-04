#include "types.h"
#include "stat.h"
#include "user.h"
#include "fs.h"

char buf[512];

void cat(char* path) {
    int n;
    int fd;

    if((fd = open(path, 0)) < 0){
        printf(2, "ls: cannot open %s\n", path);
        return;
    }

    while((n = read(fd, buf, sizeof(buf))) > 0) {
        char newBuff[512] ={0};
        char *p;
        char *p1;
        p = buf;
        p1 = buf;
        int j=0;
        for (int i = 0; i < 7; i++) {
            p = strchr(p, ':');
            p = p + 2;
            p1 = strchr(p,'\n');
            if (p1 != 0 && p != 0) {
                int howMuch = p1 - p;
                for (int l = 0; l < howMuch; l++) {
                    if (*p != '\n'){
                        newBuff[j++] = *p;
                        p++;
                    }
                }
                if (i != 6)
                    newBuff[j++] = ' ';
            }
        }
        if(strlen(newBuff) != 0)
            newBuff[j++] = '\n';
        newBuff[j] = '\0';
        if (write(1, newBuff, n) != n){
            printf(1, "cat: write error\n");
            exit();
        }
    }
    if(n < 0){
        printf(1, "cat: read error\n");
        exit();
    }
    close(fd);
}

void
lsnd(char *path){
    char buf[512], *p;
    int fd;
    struct dirent de;
    if((fd = open(path, 0)) < 0){
        printf(2, "ls: cannot open %s\n", path);
        return;
    }
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        printf(1, "ls: path too long\n");
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
        if(de.inum == 0)
            continue;
        memmove(p, de.name, DIRSIZ);
        p[DIRSIZ] = 0;
        if(strcmp(buf,"/proc/inodeinfo/.") == 0 || strcmp(buf,"/proc/inodeinfo/..") == 0)
            continue;
        cat(buf);
    }
    close(fd);
}

int main(int argc, char *argv[]){
    lsnd("/proc/inodeinfo");
    exit();
}
