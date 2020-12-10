#include "commands.h"

//Assume command line rmdir (pathname)
int rm_dir(char *pathname){
	int ino, parent_ino;
	MINODE* mip, * pmip;
	INODE* ip, * pip;
	char parent[256], child[256], path[256], name[256];
	char* cp;
	DIR* dp;

	if (pathname[0] == '/')
		dev = root->dev;
	else
		dev = running->cwd->dev;

	ino = getino(pathname);

	//grab its minode structure
	mip = iget(dev, ino);
	ip = &(mip->inode);

	if (!S_ISDIR(ip->i_mode)) {
		printf("Not of Dir type rm_dir()\n");
		return 1;
	}

	if (ip->i_links_count > 2) {
		printf("Still contains real links!\n");
		return 1;

	}

	parent_ino = findino(mip, &ino);
	pmip = iget(mip->dev, parent_ino);
	pip = &(pmip->inode);

	// ASSUME passed the above checks.
	// get parent DIR's ino and Minode (pointed by pip);
	for (int i = 0; i < 15; i++) {
		if (ip->i_block[i] != 0)
			bdalloc(mip->dev, ip->i_block[i]);
	}

	idalloc(mip->dev, ino);

	strcpy(path, pathname);
	strcpy(child, basename(pathname));

	//get rid of the child
	rm_child(pmip, child);

	//remove a link
	pip->i_links_count -= 1;

	pip->i_atime = pip->i_ctime = pip->i_mtime = time(0L);

	pmip->dirty = 1;
	iput(pmip);

	return 1;
}

int rm_child(MINODE *pmip, char *name){
    char buf[BLKSIZE], *cp, *rm_cp, temp[256];
    DIR *dp;
    int block_i, i, j, size, last_len, rm_len;

    for (i=0; i<pmip->inode.i_blocks; i++){
        if (pmip->inode.i_block[i]==0) break;
        get_block(pmip->dev, pmip->inode.i_block[i], buf);
        printf("get_block i=%d\n", i);
        printf("name=%s\n", name);
        dp = (DIR*)buf;
        cp = buf;

        block_i = i;
        i=0;
        j=0;

        while (cp + dp->rec_len < buf + BLKSIZE){
            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;

            if (!strcmp(name, temp)){
                i=j;
                rm_cp = cp;
                rm_len = dp->rec_len;
                printf("rm=[%d %s] ", dp->rec_len, temp);
            } else
                printf("[%d %s] ", dp->rec_len, temp);


            last_len = dp->rec_len;
            cp += dp->rec_len;
            dp = (DIR*)cp;
            j++; 
        }
        strncpy(temp, dp->name, dp->name_len);
        temp[dp->name_len] = 0;

        printf("[%d %s]\n", dp->rec_len, temp);
        printf("block_i=%d\n", block_i);

        //First 
        if (j==0){
            printf("First entry!\n");

            printf("deallocating data block=%d\n", block_i);
            //bdalloc the block
            bdalloc(pmip->dev, pmip->inode.i_block[block_i]); 

            for (i=block_i; i<pmip->inode.i_blocks; i++){ 
            }
        } else if (i==0) { // last entry
            cp -= last_len;
            last_len = dp->rec_len;
            dp = (DIR*)cp;
            dp->rec_len += last_len;
            printf("dp->rec_len=%d\n", dp->rec_len);
        } else { // middle entry
            size = buf+BLKSIZE - (rm_cp+rm_len);
            printf("copying n=%d bytes\n", size);
            memmove(rm_cp, rm_cp + rm_len, size);
            cp -= rm_len; 
            dp = (DIR*)cp;

            dp->rec_len += rm_len;

            strncpy(temp, dp->name, dp->name_len);
            temp[dp->name_len] = 0;
        }

        put_block(pmip->dev, pmip->inode.i_block[block_i], buf);
    }


    return 0;
}
