#include "commands.h"

extern int dev, imap, bmap, ninodes, nblocks;

int link_file(char *pathname, char *linkname)
{
    //MINODE *start;
    char cpy[40];
    strcpy(cpy, pathname);
    //char *parent = dirname(pathname);
    //printf("Parent: %s ", parent);
    //char *child = basename(cpy);
    //printf("Child: %s", child);
    // Given the pathname: FILE TO BE LINKED
    // Get the INODE of pathname and check file type
    // Need to go into dirname a/b then search for c
    // get the inode of c and check if it isn't a dir
    int oino = getino(pathname);
    if(oino == 0){
        printf("Original File does not exist\n");
        return -1;
    }
    MINODE *omip = iget(dev, oino);
    printf("parent ino = %d \n\n\n", oino);
    if(S_ISDIR(omip->inode.i_mode))
    {
        printf("Original Path is Directory: NOT ALLOWED\n");
        return -1;
    }

    // Search for linknames parents to see if they are parents
    strcpy(cpy, linkname);
    if(getino(linkname))
    {
        printf("File already exists\n");
        return -1;
    }
    char* parent = dirname(linkname);
    char *child = basename(cpy);
    // Check to see if the name of the link file does not exist
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);

    enter_name(pmip, oino, child);

    omip->inode.i_links_count++;
    omip->dirty = 1;

    iput(pmip);
    iput(omip);

    search(pmip, child);

    return 1;
}

int unlink_file(char *filename)
{
    char cpy[128];
    strcpy(cpy, filename);
    int ino = getino(filename);
    if(ino <= 0)
    {
        printf("[unlink]: File does not exist\n");
        return -1;
    }
    MINODE *mip = iget(dev,ino);
    // Check to see if ownership stands or is super user
    if(running->uid != mip->inode.i_uid || running->uid != 0)
    {
        printf("[unlink]: Permission Denied\n");
        return -1;
    }
    if(S_ISREG(mip->inode.i_mode) || S_ISLNK(mip->inode.i_mode))
    {
        char *parent = dirname(filename);
        char *child = basename(filename);
        int pino = getino(parent);
        MINODE *pmip = iget(dev, pino);
        rm_child(pmip, child);
        pmip->dirty = 1;
        iput(pmip);
        mip->inode.i_links_count--;
        if(mip->inode.i_links_count>0)
            mip->dirty=1;
        else{
            for(int i = 0; i < 12 && mip->inode.i_block[i] !=0; i++)
                bdalloc(dev, mip->inode.i_block[i]);
            idalloc(dev, ino);
        }
        iput(mip);
    }
    return 0;
}
