#include "commands.h"

char t1[9] = "xwrxwrxwr", t2[9] = "---------";

int cd(char* pathname)
{
	int ino;
	MINODE* mip;
	ino = getino(pathname);

	if (ino == 0)
		return -1;

	mip = iget(dev, ino);

	if (!S_ISDIR(mip->inode.i_mode)) {
		printf("chdir not of dirtype error\n");
		iput(mip);
		return -1;
	}
	iput(running->cwd);

	running->cwd = mip;
}

//              HEX   orginally in    Octal
//S_IFDIR 0x4000 () directory        0040000
//S_IFREG 0x8000 () regular file     0100000
//S_IFLNK 0xA000 () symbolic link    0120000
int ls_file(MINODE *mip, char *name)
{
    time_t time;
    char temp[64], l_name[128], buf[BLKSIZE];
    //Type of linux ext 
    INODE *ip = &mip->inode;
    u16 dtype;
    //Find type in HEX
    dtype = ip->i_mode & 0xF000;
    putchar('\n');
    //Check reg, dir, and link respectively
    if (dtype == 0x8000) 
        printf(" -");
    else if (dtype == 0x4000) 
        printf(" d");
    else if (dtype == 0xA000){ 
        printf(" l");
        //finding the linked name
        get_block(mip->dev, mip->inode.i_block[0], buf);
        strcpy(l_name, buf);
        put_block(mip->dev, mip->inode.i_block[0], buf);
        l_name[strlen(l_name)] = 0;
    }

    for (int i=0; i < 8; i++){
        if (ip->i_mode & (1 << i))
            putchar(t1[i]);
        else
            putchar(t2[i]);
    }
    printf(" "); 
    printf("links:%d ", ip->i_links_count); //ccount
    printf("uid:%d ", ip->i_uid); //owner
    printf("gid:%d ", ip->i_gid); //group
    printf("%6dB ", ip->i_size); 
    time = (time_t)ip->i_ctime;
    strcpy(temp, ctime(&time));
    temp[strlen(temp)-1]=0;
    printf("%s ", temp);
    printf(" ");
    printf("name= %s", name); 

    //Once again looking for if it is directory
    //SYMLINK
    if (dtype == 0xA000)
        printf(" -Link-> %s", l_name);

    iput(mip);
    
    return 0;
}

int ls_dir(MINODE *mip)
{
    char buf[BLKSIZE], temp[256], *cp;
    DIR *dp;
  
    // Assume DIR has only one data block i_block[0]
    get_block(dev, mip->inode.i_block[0], buf); 
    dp = (DIR *)buf;
    cp = buf;

    printf("mip->ino = %d\n", mip->ino);
    if (mip->ino == 0)
        return 0;
  
    while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;
	
        //printf("[%d %s]  ", dp->inode, temp); // print [inode# name]
        MINODE *fmip = iget(dev, dp->inode);
        fmip->dirty=0;
        ls_file(fmip, temp);

        cp += dp->rec_len;
        dp = (DIR *)cp;
    }
    putchar('\n');
    putchar('\n');

    iput(mip);

    return 0;
}

//Algorithm for ls
/*------------------------------
How to ls: ls [pathname] lists the information of either a directory or a file.
(1) ls_dir(dirname): use opendir() and readdir() to get filenames in the directory. For each filename,
call ls_file(filename).
(2) ls_file(filename): stat the filename to get file information in a STAT structure. Then list the STAT
information.
Since the stat system call essentially returns the same information of a minode, we can modify the
original ls algorithm by using minodes directly. The following shows the modified ls algorithm
(1). From the minode of a directory, step through the dir_entries in the data
blocks of the minode.INODE. Each dir_entry contains the inode number, ino, and
name of a file. For each dir_entry, use iget() to get its minode, as in
MINODE *mip = iget(dev, ino);
Then, call ls_file(mip, name).
(2). ls_file(MINODE *mip, char *name): use mip->INODE and name to list the file
information.
*
------------------------------*/
int ls(char *pathname)  
{
    u32 *ino = malloc(32);
    findino(running->cwd, ino);
    MINODE *curr = NULL;
    if (strlen(pathname) > 0){
        printf("path = %s\n", pathname);
        *ino = getino(pathname);
    } else {
        printf("Default root path..\n");
    }
    
    if (ino != 0){
        curr = iget(dev, *ino);
        ls_dir(curr);
    }

    return 0;
}

/*************** Algorithm of pwd ***************
if (wd == root){
    print("/")
} else{ 
    rpwd(wd);
}
***********************************************/
char *pwd(MINODE *wd){
    int cur_dev = dev;

    if (wd == root){
        printf("pwd = /");
        printf("\n");
        return 0;
    }
    printf("pwd = / ");
    rpwd(wd);
    putchar('\n');
    dev = cur_dev;

    return 0;
}

/*************** Algorithm of rpwd ***************
rpwd(MINODE *wd){
(1). if (wd==root) return;
(2). from wd->INODE.i_block[0], get my_ino and parent_ino
(3). pip = iget(dev, parent_ino);
(4). from pip->INODE.i_block[ ]: get my_name string by my_ino as LOCAL
(5). rpwd(pip); // recursive call rpwd(pip) with parent minode
(6). print "/%s", my_name;
}
***********************************************/
void rpwd(MINODE* wd)
{
	MINODE* pip;
	char buf[BLKSIZE], * cp, my_name[64];
	DIR* dp = (DIR*)buf;
	int my_ino, parent_ino;

	if (wd == root)
		return;
	
	parent_ino = findino(wd, &my_ino);
	pip = iget(dev, parent_ino);

	findmyname(pip, my_ino, my_name);

	rpwd(pip);
	
	printf("%s/", my_name);
}