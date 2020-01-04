#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

//****************** CHANGED START ************************
#define minimum(fst, snd) ((fst) < (snd) ? (fst) : (snd))

int memstart = 200;

void itoa(char *char_arr, int num){
    int j = 0;
    int size = 0;
    if (num == 0){
        char_arr[0] = '0';
        return;
    }
    while(num != 0){
        char_arr[size] = num % 10 + '0';
        num = num / 10;
        size++;
    }
    for(j = 0; j < size/2; j++){
        char tmpchar = char_arr[j];
        char_arr[j] = char_arr[size - 1 - j];
        char_arr[size - 1 - j] = tmpchar;
    }
    char_arr[size]='\0';
}

void writeStrToBuff(char *buff, char* str){
    safestrcpy(buff + strlen(buff), str ,strlen(str)+1);
}

void writeNumToBuff(char *buff, int integer){
    char string_integer[10] = {0};
    itoa(string_integer, integer);
    memmove(buff + strlen(buff), string_integer,strlen(string_integer));
}

void writeDirToBuff(char *buff, char *chars, int inum, int idx){
    struct dirent directory;
    directory.inum = inum;
    safestrcpy(directory.name,chars,strlen(chars)+1);
    memmove(buff+idx*sizeof(struct dirent),&directory,sizeof(struct dirent));
}

int makeProcFiles(char *buff){
    writeDirToBuff(buff,".",namei("/proc\0")->inum,0);
    writeDirToBuff(buff,"..",namei("")->inum,1);
    writeDirToBuff(buff,"ideinfo\0",memstart+1,2);
    writeDirToBuff(buff,"filestat\0",memstart+2,3);
    writeDirToBuff(buff,"inodeinfo\0",memstart+3,4);
    int pids[NPROC] = {0};
    getPids(pids);
    int i,j = 5;
    char arr[10] = {0};
    for (i = 0; i<NPROC; i++){
        if (pids[i] != 0){
            arr[0] = 0;
            arr[1] = 0;
            arr[2] = 0;
            arr[3] = 0;
            arr[4] = 0;
            arr[5] = 0;
            arr[6] = 0;
            arr[7] = 0;
            arr[8] = 0;
            arr[9] = 0;
            itoa(arr, pids[i]);
            writeDirToBuff(buff,arr, memstart+(i+1)*100, j);
            j++;
        }
    }
    return sizeof(struct dirent)*j;
}

int ideinfo(char *buff){
    int waiting = getWaitingOperations();
    writeStrToBuff(buff,"Waiting operations: \0");
    writeNumToBuff(buff, waiting);
    writeStrToBuff(buff,"\nRead waiting operations: \0");
    writeNumToBuff(buff, getReadingOperations());
    writeStrToBuff(buff,"\nWrite waiting operations: \0");
    writeNumToBuff(buff, getWritingOperations());
    writeStrToBuff(buff,"\nWorking blocks: \0");
    int workingBlocks[waiting*2];
    getListOfWorkingBlocks(workingBlocks);
    for(int k = 0; k<waiting*2;k=k+2){
        writeStrToBuff(buff,"(\0");
        writeNumToBuff(buff, workingBlocks[k]);
        writeStrToBuff(buff,",\0");
        writeNumToBuff(buff, workingBlocks[k+1]);
        writeStrToBuff(buff,");\0");
    }
    writeStrToBuff(buff,"\n\0");
    return strlen(buff);
}

int filestats(char *buff){
    writeStrToBuff(buff,"Free fds: \0");
    writeNumToBuff(buff,getFreeFds());
    writeStrToBuff(buff,"\nUnique inode fds: \0");
    writeNumToBuff(buff,getUniqueInodeFds());
    writeStrToBuff(buff,"\nWriteable fds: \0");
    writeNumToBuff(buff,getWritableFds());
    writeStrToBuff(buff,"\nReadable fds: \0");
    writeNumToBuff(buff,getReadableFds());
    writeStrToBuff(buff,"\nRefs per fds: \0");
    writeNumToBuff(buff,getRefsPerFds());
    writeStrToBuff(buff,"/\0");
    writeNumToBuff(buff, getTotalFds());
    writeStrToBuff(buff,"\n\0");
    return strlen(buff);
}

int inodeinfo(char *buff){
    writeDirToBuff(buff,".", namei("/proc/inodeinfo\0")->inum, 0);
    writeDirToBuff(buff,"..", namei("/proc\0")->inum, 1);
    int i =2;
    for(int j = 0;j<50;j++) {
        char arr[50] = {0};
        struct inode *ip = getINode(j);
        if (ip->ref > 0 && ip->inum!=0) {
            itoa(arr, j);
            writeDirToBuff(buff, arr, memstart + 6500 + j, i);
            i++;
        }
    }
    return sizeof(struct dirent)*i;
}

int inodeInfoData(int inum, char *buff) {
    int i=(inum-memstart-6500);
    struct inode *ip = getINode(i);
    writeStrToBuff(buff,"Device: \0");
    char tmp[1000]={0};
    writeNumToBuff(buff,ip->dev);
    writeStrToBuff(buff,tmp);
    writeStrToBuff(buff,"\nInode number: \0");
    writeNumToBuff(buff,ip->inum);
    writeStrToBuff(buff,"\nis valid: \0");
    writeNumToBuff(buff,ip->valid);
    writeStrToBuff(buff,"\ntype: \0");
    if(ip->type == T_DIR)
        writeStrToBuff(buff,"DIR\0");
    else if(ip->type == T_FILE)
        writeStrToBuff(buff,"FILE\0");
    else
        writeStrToBuff(buff,"DEV\0");
    writeStrToBuff(buff,"\nmajor minor: \0");
    writeStrToBuff(buff,"(");
    char tmp1[100]={0};
    writeNumToBuff(tmp1,ip->major);
    writeStrToBuff(buff,tmp1);
    writeStrToBuff(buff,",");
    char tmp2[100]={0};
    writeNumToBuff(tmp2,ip->minor);
    writeStrToBuff(buff,tmp2);
    writeStrToBuff(buff,")");
    writeStrToBuff(buff,"\nhard links: \0");
    writeNumToBuff(buff,ip->nlink);
    writeStrToBuff(buff,"\nblocks used: \0");
    int size;
    if(ip->type == T_DEV)
        size=0;
    else
        if (ip->size % 512 == 0)
            size = (ip->size)/512;
        else
            size = ((ip->size)/512) + 1;
    writeNumToBuff(buff,size);
    writeStrToBuff(buff,"\n");
    return strlen(buff);
}

int dirPid(char *buff){
    short proc = buff[0];
    int pid_number = getProcPidByOffset(proc);
    char location[10] = {0};
    char proc1[20]={'/','p','r','o','c','/','\0'};
    writeNumToBuff(location,pid_number);
    writeStrToBuff(proc1,location);
    writeDirToBuff(buff,".",namei(proc1)->inum, 0);
    writeDirToBuff(buff,"..",namei("/proc\0")->inum, 1);
    writeDirToBuff(buff,"status\0", memstart+2+(proc+1)*100, 2);
    writeDirToBuff(buff,"name\0", memstart+1+(proc+1)*100, 3);
    return sizeof(struct dirent)*5;
}

int handleRead(struct inode* ip, char* buff) {
    if (ip->inum < memstart)
        return makeProcFiles(buff);
    if (ip->inum == (memstart+1))
        return ideinfo(buff);
    if (ip->inum == (memstart+2))
        return filestats(buff);
    if (ip->inum == (memstart+3))
        return inodeinfo(buff);
    if (ip->inum >= (memstart+6500))
        return inodeInfoData(ip->inum,buff);
    if (ip->inum % 100 == 0)
        return dirPid(buff);
    if (ip->inum % 100 == 1)
        return getProccessName(buff);
    if (ip->inum % 100 == 2)
        return getProccessStatus(buff);
    return 0;
}

int procfsisdir(struct inode *node){
    if (node->type != T_DEV || node->major != PROCFS)
        return 0;
    int inum = node->inum;
    if (inum == (memstart+1) || inum == (memstart+2) || inum % 100 == 1 || inum % 100 == 2 || inum>=memstart+6500)
        return 0;
    return (inum < memstart || inum == (memstart+3)|| inum % 100 == 0);
}

void procfsiread(struct inode* dp, struct inode *ip){
    ip->valid = 1;
    ip->major = PROCFS;
    ip->type = T_DEV;
}

int procfsread(struct inode *ip, char *dst, int off, int n){
    char buff[1056] = {0};
    short pid = 0;
    if (ip->inum >= memstart+100 && ip->inum<memstart+6500){
        pid = (ip->inum-memstart)/100 -1;
        buff[0] = pid;
    }
    int size = handleRead(ip,buff);
    memmove(dst, buff + off, n);
    return minimum(n, size - off);
}

int procfswrite(struct inode *ip, char *buf, int n){
    return -1;
}

void procfsinit(void){
    devsw[PROCFS].isdir = procfsisdir;
    devsw[PROCFS].iread = procfsiread;
    devsw[PROCFS].write = procfswrite;
    devsw[PROCFS].read = procfsread;
}
//****************** CHANGED END ************************
