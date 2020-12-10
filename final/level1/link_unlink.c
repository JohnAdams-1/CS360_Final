#include "commands.h"

extern int dev, imap, bmap, ninodes, nblocks;

//READ Chapter 11.8.6 - 11.8.9
int link_file(char *orgFile, char *newFile)
{
    int oino, nino;
    MINODE *omip, *pip;
    char oparent[64], ochild[64], nparent[64], nchild[64];

    printf("linking %s with new %s file...\n", orgFile, newFile);
    dbname(orgFile, oparent, ochild);

    oino = getino(ochild);  
    omip = iget(dev, oino);

    if (getino(newFile) != 0) {
        printf("File %s already exists\n", newFile);
        return 0;
    }
    //Make sure correct type
    if (omip->inode.i_mode == 0x81A4) {
        DIR *dp;    
        MINODE *nmip;
        dbname(newFile, nparent, nchild);
        nino = getino(nparent);
        nmip = iget(dev, nino);
        if (nmip->dev !=  omip->dev) {
            printf("link_file failure\n");
            return 0;
        }
        if (nmip->inode.i_mode != 0x41ED) {
            printf("Not of dirtype link_file\n");
            return 0;
        }
        
        //A new file with the same inode #
        omip->dirty = 1;
        enter_name(nmip, omip->ino, nchild);
        //increase the link count for this inode
        omip->inode.i_links_count++;
        iput(omip);
        iput(nmip);
        printf("Successful Linkage\n");
        return 1;
    }
    else if (omip->inode.i_mode == 0x41ED) {
        printf("Cant link to a Directory!\n");
        iput(omip);
        return 0;
    }
    printf("not a FILE\n");
}

int unlink_file(char* filename)
{
	int ino, pino;
	char *child, *parent;
	MINODE *mip, *pmip;

	ino = getino(filename);
	if (ino == 0)
	{
		return -1;
	}
	mip = iget(dev, ino);
	//check if directory type 
	if (mip->inode.i_mode == 0x41ED)
	{
		return -1;
	}
	parent = dirname(filename);
	child = basename(filename);
	pino = getino(parent);
	pmip = iget(dev, pino);
	rm_child(pmip, child);
	pmip->dirty = 1;
	iput(pmip);
	//link count is now one less
	mip->inode.i_links_count--;
	//Dirty if link still exists
	if (mip->inode.i_links_count > 0)
	{
		mip->dirty = 1;
	}
	else
	{
		mip->refCount++;
		mip->dirty = 1;
		idalloc(dev, mip->ino);
	}
	iput(mip);
}
