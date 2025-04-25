# basefs
a basic file system for ml workload. 

basefs.h: Shared header with constants, data structures, prototypes.

basefs.c: Filesystem registration and module init/exit.

super.c: Superblock operations including mounting (fill_super) and optional saving.

inode.c: Inode operations (create, lookup, etc.).

file.c: File operations (read, write, open, release) and address space ops.

makefs.c: A user-space tool to create an empty BaseFS image file.

Makefile (kernel module build script, optional demonstration).