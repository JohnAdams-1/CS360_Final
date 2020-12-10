#include "commands.h"

int make_dir(char* name)
{
	MINODE* pip; 

	if (name[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;

	char parent[128], child[128], path1[128], path2[128];
	int ino = 0, start, mk;

	printf("DEV: %d\n", dev);

	strcpy(path1, name);
	strcpy(parent, dirname(path1));
	printf("parent: %s\n", parent);

	strcpy(path2, name);
	strcpy(child, basename(path2));
	printf("child: %s\n", child);

	ino = getino(parent);
	pip = iget(dev, ino);

	if (!pip) {
		printf("PARENT DOES NOT EXIST!\n");
		return 0;
	}

	if (!S_ISDIR(pip->inode.i_mode)) {
		printf("PARENT NOT A DIRECTPORY!\n");
		return 0;
	}

	if (getino(name) != 0)
	{
		printf("INVALID: %s ALREADY EXISTS!\n", name);
		return 0;
	}

	// 4. call mymkdir(pip, child);
	mymkdir(pip, child);

	return 1;
}

//https://eecs.wsu.edu/~cs360/mkdir_creat.html
int mymkdir(MINODE *pip, char *name){
    char buf[BLKSIZE], *cp;
    DIR *dp;
    MINODE *mip;
    INODE *ip;
    int ino = ialloc(dev), bno = balloc(dev), i;
    printf("ino=%d bno=%d\n", ino, bno);

    mip = iget(dev, ino);
    ip = &mip->inode;

    char temp[256];
    findmyname(pip, pip->ino, temp);
    printf("ino=%d name=%s\n", pip->ino, temp);

    ip->i_mode = 0x41ED; // set to dir type and set perms
    ip->i_uid = running->uid; // set owner uid
    ip->i_gid = running->gid; // set group id
    ip->i_size = BLKSIZE; // set byte size
    ip->i_links_count = 2; // . and ..
    ip->i_blocks = 2; // each block = 512 bytes
    ip->i_block[0] = bno; // new dir has only one data block
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L); // set to current time

    for (i=1; i <= 14; i++)
        ip->i_block[i] = 0;
    //write mip to disk
    // make dirty
    mip->dirty = 1; 
    
    iput(mip); // write to disk
    bzero(buf, BLKSIZE);
    cp = buf;

    dp = (DIR*)cp; 
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';

    cp += dp->rec_len;
    dp = (DIR *)cp; 

    dp->inode = pip->ino;
    dp->rec_len = BLKSIZE-12;
    dp->name_len = 2;
    //double dot
    dp->name[0] = dp->name[1] = '.';

    put_block(dev, bno, buf); 
    printf("Written to disk %d\n", bno);
    enter_name(pip, ino, name);

    return 0;
}

int enter_name(MINODE *pip, int myino, char *myname){
    char buf[BLKSIZE], *cp, temp[256];
    DIR *dp;
    int block_i, i, ideal_len, need_len, remain, blk;

    need_len = 4 * ((8 + (strlen(myname)) + 3) / 4);
    printf("Need len %d for %s\n", need_len, myname);

    for (i=0; i<12; i++){ // find empty block
        if (pip->inode.i_block[i]==0) break;
        get_block(pip->dev, pip->inode.i_block[i], buf); // get that empty block
        printf("get_block(i) where i=%d\n", i);
        block_i = i;
        dp = (DIR *)buf;
        cp = buf;

        blk = pip->inode.i_block[i];

        while (cp + dp->rec_len < buf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            printf("[%d %s] ", dp->rec_len, temp);        
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
        printf("[%d %s]\n", dp->rec_len, dp->name);

        ideal_len = 4 * ((8 + dp->name_len + 3) / 4);

        printf("ideal_len=%d\n", ideal_len);
        remain = dp->rec_len - ideal_len;

        if (remain >= need_len){
            dp->rec_len = ideal_len; // trim last rec_len to ideal_len

            cp += dp->rec_len;
            dp = (DIR*)cp;
            dp->inode = myino;
            dp->rec_len = remain;
            dp->name_len = strlen(myname);
            strcpy(dp->name, myname);
        }
    }

    printf("put_block : i=%d\n", block_i);
    put_block(pip->dev, pip->inode.i_block[block_i], buf);
    printf("put parent block to dist=%d to disk\n", blk);

    return 0;
}

int creat_file(char *pathname){
    char *path, *name, cpy[128];
    int pino, r, dev;
    MINODE *pmip;

    strcpy(cpy, pathname); // dirname/basename destroy pathname, must make copy

    if (abs_path(pathname)==0){
        pmip = root;
        dev = root->dev;
    } else {
        pmip = running->cwd;
        dev = running->cwd->dev;
    }

    path = dirname(cpy);
    name = basename(pathname);

    //printf("path=%s\n", path);
    //printf("filename=%s\n", name);

    pino = getino(path);
    pmip = iget(dev, pino);
    
    if(!pmip)
    {
        printf("no parent returned\n");
        iput(pmip);
        return 0;
    }
    //Checking if is dir
    if ((pmip->inode.i_mode & 0xF000) == 0x4000){ // is_dir
        if (search(pmip, name)==0){ // if can't find child name in start MINODE
            r = mycreat(pmip, name);
            pmip->inode.i_atime = time(0L); // touch atime
            pmip->dirty = 1; // make dirty
            iput(pmip); // write to disk

            printf("Successfully created file %s\n", pathname);
            return 1;
        } else {
            iput(pmip);
        }
    }

    return 0;
}

int mycreat(MINODE *pip, char *name){
    MINODE *mip;
    int ino, bno, i;
    char *cp, buf[BLKSIZE];
    DIR *dp;

    ino = ialloc(dev); 
    bno = balloc(dev); 

    mip = iget(dev, ino);
    INODE *ip = &mip->inode;

    ip->i_mode = 0x81A4;		
    ip->i_uid  = running->uid;	 
    ip->i_gid  = running->gid;
    ip->i_size = 0;		
    ip->i_links_count = 1;	       
    ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);  
    ip->i_blocks = 0;                	
    ip->i_block[0] = bno;             
    for (i=1; i<15; i++){
        ip->i_block[i] = 0;
    }

    mip->dirty = 1;               
    iput(mip);                    

    enter_name(pip, ino, name);
}
