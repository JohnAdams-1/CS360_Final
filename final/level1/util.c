/*********** util.c file ****************/
#include "util.h"

extern PROC *running;
extern MINODE *root, minode[NMINODE];
extern MTABLE mtable[NMTABLE];
extern char gline[128], *name[64], dname[64], bname[64];
extern int nname, inode_start, dev, imap, bmap, ninodes, nblocks;
int r;

//Given from book, reads from the lseek on the buf. put_block will write to it. 
int get_block(int dev, int blk, char *buf){
    lseek(dev, (long)blk*BLKSIZE, 0);
    r = read(dev, buf, BLKSIZE);

    return r;
}   

int put_block(int dev, int blk, char *buf){
    lseek(dev, (long)blk*BLKSIZE, 0);
    r = write(dev, buf, BLKSIZE);

    return r;
}   

//FROM THE BOOK
int tokenize(char *pathname){
    printf("TOKENIZING %s...", pathname);
    int i;
    char *s;
    strcpy(gline, pathname);   // tokens are in global gline[ ]
    nname = 0;

    s = strtok(gline, "/");
    while(s){
        name[nname++] = s;
        s = strtok(0, "/");
    }
    for (i= 0; i<nname; i++)
        printf("%s  ", name[i]);
    printf("\n");

    return 0;
}

//3. MINODE *iget(dev, ino):
//return mip->minode[] containing INODE of (dev,ino)
MINODE *iget(int dev, int ino){
    int i;
    MINODE *mip;
    char buf[BLKSIZE];
    int blk, offset;
    INODE *ip;

    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        if (mip->dev == dev && mip->ino == ino){
            //printf("\nmip=%s refCount++ now=%d\n", mip->ino, mip->refCount);
            mip->refCount++;
            //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
            return mip;
        }
    }
    
    for (i=0; i<NMINODE; i++){
        mip = &minode[i];
        if (mip->refCount == 0){
            //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
            mip->refCount = 1;
            mip->dev = dev;
            mip->ino = ino;

            // get INODE of ino into buf[ ]    
            blk    = (ino-1)/8 + inode_start;
            offset = (ino-1) % 8;

            //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

            get_block(dev, blk, buf);
            ip = (INODE *)buf + offset;
            // copy INODE to mp->INODE
            mip->inode = *ip;
            return mip;
        }
    }   
    printf("Error in iget\n");
    return 0;
}
    
void iput(MINODE *mip){
    int block, offset;
    char buf[BLKSIZE];
    INODE *ip;

    if (mip==0) return;

    //printf("\nmip=%d refCount-- now=%d\n", mip->ino, mip->refCount);
    mip->refCount--;
    if (mip->refCount > 0)  // minode is still in use
        return;
    if (!mip->dirty)        // INODE has not changed; no need to write back
        return;

    printf("inode_start is %d\n", inode_start);

    block = (mip->ino - 1) / 8 + inode_start;
    offset = (mip->ino -1) % 8;

    printf("block is %d and offset is %d\n", block, offset);

    get_block(mip->dev, block, buf);
    ip = (INODE *)buf + offset;
    *ip = mip->inode;
    put_block(mip->dev, block, buf);
    mip->refCount = 0;
} 

//FROM BOOK BUT EDITED HOW IT LOOKS 
//SEARCH DIRECTORIES USING MINODE
int search(MINODE* mip, char* name)
{
	char* cp, buf[BLKSIZE], temp[256];
	int i;
	DIR* dp;
	INODE* ip;

	//printf("search for %s in MINODE = [%d, %d]\n", name, mip->dev, mip->ino);
	ip = &(mip->inode);
	//search for name in mip's data blocks: ASSUME i_block[0] 
	for (i = 0; i < 12; i++) {
		if (ip->i_block[i] == 0)
			break;

		get_block(dev, ip->i_block[i], buf);
		cp = buf;
		dp = (DIR*)cp;
		while (cp < buf + BLKSIZE) {
			strncpy(temp, dp->name, dp->name_len);
			temp[dp->name_len] = 0;
			printf("inode:%2d re_len:%2d name_len:%2u name= %s\n", dp->inode, dp->rec_len, dp->name_len, temp);
			if (strcmp(name, temp)==0){
                printf("\033[0;32m");
                printf("\nFound %s at inode #%d\n", name, dp->inode);
                printf("\033[0m");
                return dp->inode;
            }
			cp += dp->rec_len;
			dp = (DIR*)cp;
		}
	} 
	printf("%s DOES NOT EXIST...\n", name);
	return 0;
}

//From book but had to find whether it was directory or not
//we used same implementation as in the given ls_file code by
//checking that (inode.i_mode & 0xF000) == 0x4000
int getino(char *pathname)
{
    MINODE *mip;
    int i, ino;

    if (strcmp(pathname, "/")==0){
    return 2;
    // return root
    }
    if (pathname[0] == "/"){
        mip = root;
    }
    // if absolute
    else{
        mip = running->cwd;
    }
    // if relative
    mip->refCount++;
    // in order to
    tokenize(pathname);
    iput(mip); 
    // assume: name[ ], nname are globals
    for (i=0; i<nname; i++){
        // search for each component string
        //S_ISDIR(mip->inode.i_mode in textbook, we need to prove it is not a Dir Type
        if (!(mip->inode.i_mode & 0xF000) == 0x4000){ // check DIR type
            printf("%s is not a directory\n", name[i]);
            iput(mip);
            return 0;
        }
        ino = search(mip, name[i]);
        if (!ino){
            printf("no such component name %s\n", name[i]);
            iput(mip);
            return 0;
        }
        iput(mip);
        // release current minode
        mip = iget(dev, ino);
    // switch to new minode
    }
    iput(mip);
    return ino;
}
    
int findmyname(MINODE *parent, u32 myino, char *myname){
    DIR *dp;
    char buf[BLKSIZE], temp[256], *cp;

    get_block(dev, parent->inode.i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;

    while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        if (dp->inode == myino)
            strcpy(myname, temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }

    return 0;
}

//find ino of the MINODE structure from entry in "INODE.i_block[0]"
int findino(MINODE *mip, u32 *myino){ // myino = ino of . return ino of ..
    char buf[BLKSIZE], *cp;   
    DIR *dp;

    //We take the mip pass in it's ->dev and "INODE.i_block[0]" get_block
    get_block(mip->dev, mip->inode.i_block[0], buf);
    cp = buf; 
    dp = (DIR *)buf;
    *myino = dp->inode;
    cp += dp->rec_len;
    dp = (DIR *)cp;
    return dp->inode;
}


int show(MINODE *mip){
    DIR *dp;
    char buf[BLKSIZE], name[256], *cp;
    INODE *ip = &(mip->inode);

    get_block(mip->dev, ip->i_block[0], buf);
    cp = buf;
    dp = (DIR*)cp;

    printf("\ninode\trec_len\tname_len\tname\n========================================\n");
    while (cp < buf + BLKSIZE){
        strncpy(name, dp->name, dp->name_len);
        name[dp->name_len]=0;

        printf("%1d\t%1d\t%1d\t%s\n", dp->inode, dp->rec_len, dp->name_len, name);

        cp += dp->rec_len;
        dp = (DIR*)cp;
    }
    putchar('\n');

    return 0;
}

// LEVEL 1 PAGE 333 KC WANG SYSTEMS PROGRAMMING
int tst_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    if (buf[i] & (1 << j))
        return 1;

    return 0;
}

int dec_free_inodes(int dev){
    //dec free inodes count in SUPER and GD
    char buf[BLKSIZE];

    get_block(dev, 1, buf);
    sp = (SUPER *)buf;
    sp->s_free_inodes_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; 
    gp->bg_free_inodes_count--;
    put_block(dev, 2, buf);

    return 0;
}

int abs_path(char *path){
    if (path[0] == '/')
        return 0;
    else
        return -1;
}

int set_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    buf[i] |= (1 << j);

    return 0;
}

int clr_bit(char *buf, int bit){
    int i = bit / 8, j = bit % 8;

    buf[i] &= ~(1 << j);

    return 0;
}

int dec_free_blocks(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // dec the super table
    sp = (SUPER *)buf;
    sp->s_free_blocks_count--;
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_blocks_count--;
    put_block(dev, 2, buf);

    return 0;
}

//Page 338: int idealloc(dev, ino) deallocates an inode (number)
int ialloc(int dev){
    int i;
    char buf[BLKSIZE];

    get_block(dev, imap, buf);

    for (i=0; i < ninodes; i++){
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, imap, buf);
            dec_free_inodes(dev);
            return i+1;
        }
    }
    return 0;
}

int inc_free_inodes(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); // get the super table
    sp = (SUPER*)buf;
    sp->s_free_inodes_count++; // inc free inodes
    put_block(dev, 1, buf); // put it back

    get_block(dev, 2, buf); // get the gd table
    gp = (GD*)buf;
    gp->bg_free_inodes_count++; // inc free inodes
    put_block(dev, 2, buf); // put it back

    return 0;
}

int inc_free_blocks(int dev){
    char buf[BLKSIZE];

    get_block(dev, 1, buf); 
    sp = (SUPER *)buf;
    sp->s_free_blocks_count++; // dec the super table
    put_block(dev, 1, buf);

    get_block(dev, 2, buf);
    gp = (GD *)buf; // dec the GD table
    gp->bg_free_blocks_count++;
    put_block(dev, 2, buf);

    return 0;
}


int idalloc(int dev, int ino){
    char buf[BLKSIZE];

    if (ino > ninodes){
        printf("inumber=%d not possible.\n", ino);
        return 0;
    }

    get_block(dev, imap, buf);
    clr_bit(buf, ino-1);
    put_block(dev, imap, buf);

    inc_free_inodes(dev);
    printf("deallocated ino with %d\n", ino);

    return 0;
}

int balloc(int dev){
    int i;
    char buf[BLKSIZE];

    get_block(dev, bmap, buf);

    for (i=0; i < nblocks; i++){
        if (tst_bit(buf, i) == 0){
            set_bit(buf, i);
            put_block(dev, bmap, buf);
            printf("allocated block in balloc is %d\n", i+1); // bits 0..n, ino 1..n+1
            dec_free_blocks(dev);
            return i+1;
        }
    }
    return 0;
}

int bdalloc(int dev, int blk){
    char buf[BLKSIZE];

    if (blk > nblocks){
        printf("bdalloc error\n");
        return 0;
    }

    get_block(dev, bmap, buf);
    clr_bit(buf, blk-1);
    put_block(dev, bmap, buf);

    inc_free_blocks(dev);
    printf("deallocated block is %d\n", blk-1);

    return 0;
}

//decompose a pathname into the dir_name and base_name where pathname=a/b/c with dir_name=/a/b and base_name=c
//Uses global strings dname and bname
int dbname(char *pathname, char *dname, char *bname)
{
    char temp[128]; // dirname(), basename() destroy original pathname
    strcpy(temp, pathname);
    strcpy(dname, dirname(temp));
    strcpy(temp, pathname);
    strcpy(bname, basename(temp));
}

int faccess(char *pathname, char mode)
{
    char t1[9] = "xwrxwrxwr", t2[9] = "---------";
    char permi[9];
    int offset = 0;
    int ino = getino(pathname);
    MINODE *mip = iget(dev,ino);
    INODE *ip = &mip->inode;

    for (int i=8; i >= 0; i--) // permissions
        if (ip->i_mode & (1 << i))
            permi[i]=t1[i];
        else
            permi[i]='-';
    
    //printf("\npermission %s\n", permi);
    if(mode == 'w')
        offset = 1;
    if(mode == 'x')
        offset = 2;

    // Super User
    if(running->uid ==0)
        return 1;

    // Owner
    if(ip->i_uid == running->uid)
        if(mode == permi[offset])
            return 1;
    // Same group
    else if(ip->i_gid == running->gid)
        if(mode == permi[offset + 3])
            return 1;
    // Other
    else
        if(mode == permi[offset + 6])
            return 1;

    return 0;
}

int maccess(MINODE *mip, char mode)
{
    char t1[9] = "xwrxwrxwr", t2[9] = "---------";
    char permi[9];
    int offset = 0;
    INODE *ip = &mip->inode;

    for (int i=8; i >= 0; i--) // permissions
        if (ip->i_mode & (1 << i))
            permi[i]=t1[i];
        else
            permi[i]='-';
    
    //printf("\npermission %s\n", permi);
    if(mode == 'w')
        offset = 1;
    if(mode == 'x')
        offset = 2;

     // Super User
    if(running->uid ==0)
        return 1;    

    // Owner
    if(ip->i_uid == running->uid)
        if(mode == permi[offset])
            return 1;
    // Group
    else if(ip->i_gid == running->gid)
        if(mode == permi[offset + 3])
            return 1;
    // Other
    else
        if(mode == permi[offset + 6])
            return 1;

    return 0;
}