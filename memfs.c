#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>


#include "memfs_c.h"

static struct fuse_operations operations = 
{
    // .lookup = my_lookup,
    .getattr = my_getattr,
    .readlink = my_readlink,
    .mknod = my_mknod,
    .mkdir = my_mkdir,
    .symlink = my_symlink,
    // .open = my_open,
    .read = my_read,
    .write = my_write,
    .readdir = my_readdir,
    .init = my_init,
    // .create = my_create,
};

int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &operations, NULL);
}
