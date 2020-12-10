#include <libgen.h>
#include <time.h>

#include <sys/stat.h>

#include "util.h"

//global vars from main
extern MINODE minode[NMINODE], *root;
extern PROC proc[NPROC], *running;
extern MTABLE mtable[NMTABLE];
extern int dev, imap, bmap, ninodes, nblocks;
//Needed for open_file
extern OFT oft[NOFT];
int open_file(char *pathname, char *mode);
int cp_file(char *src, char *dest);

int myread(int fd, char *buf, int nbytes);


//main.c
int init();
int mount_root();
int quit();

//cd_ls_pwd.c
int cd(char *pathname);
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *mip);
int ls(char *pathname);
char *pwd(MINODE *wd);
void rpwd(MINODE *wd);

//mkdir_creat.c
int make_dir(char *pathname);
int mymkdir(MINODE *pip, char *name);
int enter_name(MINODE *pip, int myino, char *myname);
int creat_file(char *pathname);
int mycreat(MINODE *pip, char *name);

//rmdir.c
int rm_dir(char *pathname);
int rm_child(MINODE *pmip, char *name);

//link_unlink.c
int link_file(char *pathname, char *linkname);
int unlink_file(char *filename);
int sym_link(char *src, char *dest);

int close_wrapper(char* fd);
int close_file(int fd);
int pfd(void);