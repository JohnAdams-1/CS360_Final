#include "../level1/commands.h"

int mv_file(char *src, char *dest)
{
    int sfd = open_file(src, "0");
    MINODE *mip = running->fd[sfd]->minodePtr;
    // Same Dev link then remove src
    if(mip->dev == sfd)
    {
        printf("\n\nSame Dev\n\n\n\n");
        link_file(src, dest);
        unlink_file(src);
    }
    // Different dev use cp
    else
    {
        printf("Different Dev\n");
        cp_file(src, dest);
        unlink_file(src);
    }
    
    return 0;
}
