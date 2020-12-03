#include "../level1/commands.h"

int write_file()
{
    char string[BLKSIZE];
    int fd;
    pfd();
    printf("[write_file]: enter fd = ");
    scanf("%d", &fd);
    printf("[write_file]: enter text = ");
    scanf("%s", string);
    if(fd < 0 || fd >=NFD)
        return -1;
    if(running->fd[fd] == NULL)
        return -1;
    if(running->fd[fd]->mode == 1 || running->fd[fd]->mode == 2)
        return(mywrite(fd, string, strlen(string)));
    printf("[write_file]: Can't write to a file designated for read\n");
    return -1;
}

int mywrite(int fd, char buf[], int nbytes)
{
    printf("[mywrite]: %d %d\n", (int)strlen(buf), nbytes);
    int count = nbytes, blk, dblk;
    int ibuf[256], dbuf[256], buf13[256];
    char wbuf[BLKSIZE];
    OFT *oftp;
    oftp = running->fd[fd];
    MINODE *mip;
    mip = oftp->minodePtr;
    while(nbytes > 0)
    {
        // Logical block
        int lbk = oftp->offset / BLKSIZE;
        // Start Byte
        int startByte = oftp->offset % BLKSIZE;
        // Direct Block
        if(lbk <12)
        {
            printf("[mywrite]: Direct Block\n");
            // If no data block is present, allocate one
            if(mip->inode.i_block[lbk] == 0)
                mip->inode.i_block[lbk] = balloc(mip->dev);
            // Saves Block into blk
            blk  = mip->inode.i_block[lbk];
        }
        // Indirect Block
        else if(lbk >= 12 && lbk < 256 + 12)
        {
            printf("[mywrite]: Indirect Block\n");
	        // Like above, if no data block allocate one
            if(mip->inode.i_block[12] == 0)
            {
                // Allocate block
                mip->inode.i_block[12] = balloc(mip->dev);
                // Get block into memmory
                get_block(mip->dev, mip->inode.i_block[12], (char *)ibuf);
                // Zero it out
                bzero(ibuf, 256);
                //memset(ibuf, 0, 256);
                // Put block into memory
                put_block(mip->dev,mip->inode.i_block[12], (char *)ibuf);
            }
            // Get block into ibuf
            get_block(mip->dev, mip->inode.i_block[12], (char *)ibuf);
            // Set blk to disk block
            blk = ibuf[lbk -12];
            // If 0 allocate it
            if(blk ==0)
            {
                mip->inode.i_block[12] = balloc(mip->dev);
                ibuf[lbk -12] = mip->inode.i_block[lbk];
            }
        }
        // Double indirect blocks
        else
        {
            // From the help posted by KC
            printf("[mywrite]: Double Indirect Block\n");
            lbk -= (256+12);
            get_block(mip->dev,mip->inode.i_block[13], (char *)buf13);
            dblk = buf13[lbk/256];
            get_block(mip->dev, dblk, (char *)dbuf);
            blk = dbuf[lbk %256];
        }
        // Reset wbuf as to not add too many characters in the last buffer
        memset(wbuf, 0, BLKSIZE);
        // Get data block blk into wbuf
        get_block(mip->dev, blk, wbuf);
        // Set the start point of wbuf
        char *cp = wbuf + startByte;
        int remain = BLKSIZE -startByte;
        char *cq = buf; 
        // Write until remain == 0
        while(remain > 0)
        {
            *cp++ = *cq++;
            nbytes--; remain--;
            oftp->offset++;
            // If offset is greater than size increase file size
            if(oftp->offset > mip->inode.i_size)
                mip->inode.i_size++;
            // Break when you have read all the bytes
            if(nbytes <= 0)
                break;
        }
        // Put wbuf into data block blk
        put_block(mip->dev,blk,wbuf);
    }
    // Mark as dirty to save when done
    mip->dirty =1;
    printf("[mywrite]: Wrote %d char into file descriptor fd=%d\n", count,fd);
    return nbytes;
}
