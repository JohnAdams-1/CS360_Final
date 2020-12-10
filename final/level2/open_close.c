#include "../level1/commands.h"
//need bdalloc
#include "../level1/util.h"
//#include "type.h"
//need NOFT 
extern OFT oft[NOFT];
//lseek can be in here
int open_file(char *pathname, char *mode){
    OFT *open;
    MINODE *mip;
    int i, ino, i_mode = atoi(mode);

    printf("[open_file]: i_mode=%d\n", i_mode);

    if (pathname[0] == '/')
        dev = root->dev;
    else
        dev = running->cwd->dev;

    ino = getino(pathname);
    mip = iget(dev, ino); // get the ino and mip of pathname


    if(i_mode == 0)
        if (!maccess(mip, 'r'))
        { // char = 'r' for R; 'w' for W, RW, APPEND
            iput(mip); 
            return -1;
        }
    else
    {
        if (!maccess(mip, 'w'))
        { // char = 'r' for R; 'w' for W, RW, APPEND
            iput(mip); 
            return -1;
        }
    }

    if ((mip->inode.i_mode & 0xF000) != 0x8000) // check if regular file
        return -1;

    // check if file is already open with incompatible mode
    if (i_mode > 0){ // if not open for read
        for (i=0; i<NFD; i++){
            if (running->fd[i] != NULL && running->fd[i]->minodePtr == mip){
                if (running->fd[i]->mode > 0){
                    printf("[open_file]: %s already open for write!\n\n", pathname);
                    return -1;
                }
            } else
                break;
        }
    }
    
    open = (OFT*)malloc(sizeof(OFT)); // build the open fd
    open->mode = i_mode;
    open->refCount = 1;
    open->minodePtr = mip;

    switch (i_mode){ // offset
        case 0: // Read: offset = 0
            open->offset = 0; 
            break;
        case 1: // Write: truncate file to size=0
            truncate_file(mip);
            open->offset = 0;
            break;
        case 2: // Read/Write: Don't truncate, offset = 0
            open->offset = 0;
            break;
        case 3: // Append: offset to size of file
            open->offset = mip->inode.i_size;
            break;
        default:
            printf("[open_file]: Invalid mode!\n");
            return -1;
    }

    for (i=0; i<NFD; i++) // find empty fd in running PROC's fd array
        if (running->fd[i] == NULL){
            running->fd[i] = open;
            break;
        }
    
    mip->inode.i_atime = time(0L); // update inode access time

    if (i_mode > 0) // if not R then update modify time as well
        mip->inode.i_mtime = time(0L);

    mip->dirty = 1;

    return i; // return fd i
}



//DOES CLOSE_FILE() ABOVE this can be fd digit
int close_wrapper(char* fd){
    int x = 0, y = 0;
    x = atoi(fd);
    switch(x){
        case 0:
            y = 1;
        break;
        case 1:
            y = 1;
        break;
        case 2:
            y = 1;
        break;
        case 3:
            y = 1;
        break;
        default:
            printf("close_file ERROR Invalid file descriptor!\n");
        return y;
        break;
    }
    if(y){
        printf("close_file was successful\n");
        close_file(x);
        return 1;
    }
    return 0;
}

int close_file(int fd){
    OFT *pOft; 
    MINODE *pMin;
  //  int temp_fd = atoi(fd);

    if(running->fd[fd] != NULL){
        pOft = running->fd[fd];
        //swap
        running->fd[fd] = 0;
        pOft->refCount--;
        if (pOft->refCount > 0){
            return 0;
        }else{
            //not last
            pOft->mode = 0;
            pOft->offset = 0;
            //refCount is zero already
            pMin = pOft->minodePtr;
            //to disk
            iput(pMin);
            //remove the minode pointer
            pOft->minodePtr = 0;
        }
        //if last user of this OFT entry dispose the minode
    }
    //printf("Succesfully closed file\n");
    return 0;
}

int truncate_file(MINODE *mip){
    INODE *ip = &mip->inode;      
    for (int i=0; i<ip->i_blocks; i++){ //iterate through blocks
        if (ip->i_block[i] != 0){
            bzero(ip->i_block, BLKSIZE); 
            //zero it if it's not empty
        }
        else{
            break;
        }
    }
    
    ip->i_mtime = time(0L);
    ip->i_size = 0;
    mip->dirty = 1;

    return 0;
}

//need pfd to check open files
/*This function displays the currently opened files as follows:

        fd     mode    offset    INODE
       ----    ----    ------   --------
         0     READ    1234   [dev, ino]  
         1     WRITE      0   [dev, ino]
      --------------------------------------
  to help the user know what files has been opened.*/
//lots of short circuit evaluation
int pfd(void){
    MINODE *pMin;
    OFT *pOft;
    printf(" fd  mode  offset  inode| MODE: 0-READ, 1-WRITE\n");
    printf(" --  ----  ------  -----|       2-R/W , 3-APPEND\n");
    for(int i = 0; i<5; i++){
        if(running->fd[i] != NULL){
            pOft = running->fd[i];
            pMin = pOft->minodePtr;
            if(pOft != NULL && pMin != NULL){
                if(pOft->refCount > 0){
                    printf("%2d %4d %5d     [%d, %d]\n", i, pOft->mode, pOft->offset, pMin->dev, pMin->ino);
                }
            }
        }
    }
    printf("------------------------|\n");
}

