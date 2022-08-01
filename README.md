# Implement file system operations using fuse

The goal of this assigment is to implement a filesystem using [fuse](https://en.wikipedia.org/wiki/Filesystem_in_Userspace). Fuse allows one
to create their own file system without editing kernel code. On successful
completion of this excercise you will have a good understanding of filesystem
data structures and how filesystem operations are mapped onto these data structures.

## The build system

Each assignment comes with a template [Makefile](Makefile) as the make build system that
needs to be adapted depending on the programming language.
All assignments will try to build the `all` target within the Makefile like this:

```console
$ make all
```

So make sure your that your `all` target will produce all executables required for the
tests. 

At the time of writing the following set up is installed:

- C/C++ compilers: gcc (different versions from 7. to 10 i.e. gcc-10), clang 6-9
- C/C++ build systems: cmake: 3.19.6 autoconf: 2.69, automake: 1.15.1
- fuse: 2.9.9-3

## Tests

Our tests will lookup exectuables in one of the root directories.

After that it runs individual tests coming from the `tests/` folder (test
scripts are prefixed with `test_`).
Each test program is a python3 script and can be run individually, i.e.:

```console
python3 ./tests/test_mount.py
```

For convenience our Makefile also comes with `check` target which will run all tests in serial:

```console
$ make check
```


``` console
# This will mount your current directory as /code into the container
 docker run -ti --entrypoint=/bin/bash -v $(pwd):/code --rm ghcr.io/ls1-courses/ls1-runner:latest
```

## The assignment for this week

Your task is to implement an **in-memory** filesystem, by implementing the filesystem's data structures and
file operations that correctly manipulate them. Please note that you will be graded against the correctness of your implementation only. We do not check for performance. However, it is a good practice to figure out what data structures are necessary to efficiently support the respective file operations. The filesystem should be implemented with the following constraints:
- Maximum file name length of 255 ascii characters.
- Maximum file size of 512 bytes.

For rust implementations: You can find useful depedencies in `Cargo.toml`. Also, set the `fuser::MountOption::FSName` to `memfs`.

### Deliverables

1. A mountable filesystem with the correct file type (directory). It should be possible to mount the filesystem as follows:

``` console
./memfs [mount point]
```

Where the argument "mount point" is the directory where the filesystem will be mounted at.

2. The ability to create flat files and directories in the in-memory filesystem i.e, all files and directories stored in a single directory which is the root of the filesystem.

3. The ability to create hierarchical files and directories in the in-memory filesystem i.e, create files and directories in directories inside the root directory of the filesystem.

4. The ability to write data to and read data from files.

5. The ability to append data to an existing file.

6. The ability to create symlinks to files

7. Additionally, the filesystem needs to return correct filesizes after write operations

You are free to choose any data structure to manage filesystem data and metadata. To ensure that the filesystem has the above functionality the following functions must be implemented from the fuse API:

- lookup (mandatory if you implement based on a low level interface with inodes)
- getattr
- read
- write
- readdir
- mkdir
- mknod
- open
- create
- readlink
- symlink

