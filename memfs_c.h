#pragma once

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define DEBUG
#define MAXNAMELENGTH 256 // the name is of max 256 bytes
#define MAXFILELENGTH 512 // the file is of max 512 bytes
#define MAXNUM 2048       // assume there are at most 2048 files

typedef struct
{
    int valid;         // 0 if not allocated
    int type;          // 0: dir, 1: file
    int16_t name_size; // max name size of 256 bytes
    int target_idx;    // blk location in the corresponding global list (dir || file)
} Map;

typedef struct
{
    char name[MAXNAMELENGTH]; // file/directory name
    int16_t name_size;        // max name size of 256 bytes
    char data[MAXFILELENGTH]; // char date in a file
    int16_t data_size;        // max file size of 512 bytes
    int is_symlink;           // is symbolic link or not
} File;

typedef struct
{
    char name[MAXNAMELENGTH];   // file/directory name
    int16_t name_size;          // max name size of 256 bytes
    Map directory_data[MAXNUM]; // mapping for children file/dir
    int children_size;          // number of children
} Dir;

// global var
Dir dir_list[MAXNUM];
int dir_idx = -1;

File file_list[MAXNUM];
int file_idx = -1;

void *my_init(struct fuse_conn_info *conn);
int my_getattr(const char *, struct stat *);
int my_read(const char *, char *, size_t, off_t,
            struct fuse_file_info *);
int my_write(const char *, const char *, size_t, off_t,
             struct fuse_file_info *);
int my_readdir(const char *, void *, fuse_fill_dir_t, off_t,
               struct fuse_file_info *);
int my_mkdir(const char *, mode_t);
int my_mknod(const char *, mode_t, dev_t);
int my_open(const char *, struct fuse_file_info *);
int my_create(const char *, mode_t, struct fuse_file_info *);
int my_readlink(const char *, char *, size_t);
int my_symlink(const char *, const char *);

// helper for hieriachel
char **split(const char *path, int *size); // bug-free
int search_dir_from_path_piece(char **splitted, int size, int root, int pivot);
int add_dir(const char *dirname);
int add_file(const char *filename);
int get_file_idx(const char *filename);
int get_dir_idx(const char *dirname);

// debug helper
void display_list()
{
    printf("new round========================\n");
    for (int i = 0; i <= dir_idx; ++i)
        printf("%s %d \n", dir_list[i].name, dir_list[i].children_size);
    printf("======================================\n");
    for (int i = 0; i <= file_idx; ++i)
        printf("%s %d %d %s \n", file_list[i].name, file_list[i].is_symlink, file_list[i].data_size, file_list[i].data);
    printf("======================================\n");
}

// add a dir entry of dirname in the global list
int add_dir(const char *dirname)
{
    int size;
    char *dirbuf = strdup(dirname);
    char **splitted;
    splitted = split(dirbuf, &size);
    splitted[0] = "/";
    int parent_idx = search_dir_from_path_piece(splitted, size - 1, 0, 1);
    if (parent_idx < 0)
    {
        errno = ENOENT;
        return -1;
    }; // parent path not exist

    // fill the parent dir entry in dir_list
    Dir parent = dir_list[parent_idx];
    for (int i = 0; i < parent.children_size; ++i)
    {
        Map current_map = parent.directory_data[i];
        if (current_map.valid == 0)
            continue;
        if ((current_map.type == 0 && strcmp(splitted[size - 1], dir_list[current_map.target_idx].name) == 0) || (current_map.type == 1 && strcmp(splitted[size - 1], file_list[current_map.target_idx].name) == 0))
        {
            errno = EEXIST;
            return -1;
        }
    }
    dir_list[parent_idx].children_size++;
    Map mapping_to_add =
        {
            .valid = 1,
            .type = 0,
            .name_size = strlen(splitted[size - 1]),
            .target_idx = dir_idx + 1,
        };
    dir_list[parent_idx].directory_data[dir_list[parent_idx].children_size - 1] = mapping_to_add;

    // fill the target dir entry in dir_list
    dir_idx++;
    strcpy(dir_list[dir_idx].name, splitted[size - 1]);
    dir_list[dir_idx].children_size = 0;

    return 0;
}

// add a file entry of filename in the global list
int add_file(const char *filename)
{
    // search parent dir index
    int size;
    char *buf = strdup(filename);
    char **splitted;
    splitted = split(buf, &size);
    splitted[0] = "/";
    int parent_idx = search_dir_from_path_piece(splitted, size - 1, 0, 1);
    if (parent_idx < 0)
    {
        errno = ENOENT;
        return -1;
    }; // parent path not exist

    // fill the parent dir entry in dir_list
    Dir parent = dir_list[parent_idx];
    for (int i = 0; i < parent.children_size; ++i)
    {
        Map current_map = parent.directory_data[i];
        if (current_map.valid == 0)
            continue;
        if ((current_map.type == 0 && strcmp(splitted[size - 1], dir_list[current_map.target_idx].name) == 0) || (current_map.type == 1 && strcmp(splitted[size - 1], file_list[current_map.target_idx].name) == 0))
        {
            errno = EEXIST;
            return -1;
        }
    }
    dir_list[parent_idx].children_size++;
    Map mapping_to_add =
        {
            .valid = 1,
            .type = 1,
            .name_size = strlen(splitted[size - 1]),
            .target_idx = file_idx + 1,
        };
    dir_list[parent_idx].directory_data[dir_list[parent_idx].children_size - 1] = mapping_to_add;

    // fill the target file entry in file_list
    file_idx++;
    strcpy(file_list[file_idx].name, splitted[size - 1]);
    strcpy(file_list[file_idx].data, "");
    file_list[file_idx].is_symlink = 0;
    file_list[file_idx].data_size = 0;

    return 0;
}

int search_dir_from_path_piece(char **splitted, int size, int root, int pivot)
{
    if (pivot >= size)
        return root;
    char *target = splitted[pivot];
    Dir root_dir = dir_list[root];
    for (int i = 0; i < root_dir.children_size; ++i)
    {
        Map child = root_dir.directory_data[i];
        if (child.valid == 1 && strcmp(dir_list[child.target_idx].name, target) == 0 && child.type == 0)
        {
            return search_dir_from_path_piece(splitted, size, child.target_idx, pivot + 1);
        }
    }
    return -1;
}

// "/foo/foobar/file.txt"
// 1: 2: foo 3: foobar 4: file.txt
char **split(const char *path, int *size)
{
    char *tmp;
    char **splitted = NULL;
    int i, length;

    if (!path)
    {
        goto Exit;
    }

    tmp = strdup(path);
    length = strlen(tmp);

    *size = 1;
    for (i = 0; i < length; i++)
    {
        if (tmp[i] == '/')
        {
            tmp[i] = '\0';
            (*size)++;
        }
    }

    splitted = (char **)malloc(*size * sizeof(*splitted));
    if (!splitted)
    {
        free(tmp);
        goto Exit;
    }

    for (i = 0; i < *size; i++)
    {
        splitted[i] = strdup(tmp);
        tmp += strlen(splitted[i]) + 1;
    }
    return splitted;

Exit:
    *size = 0;
    return NULL;
}

// get the index of a file in the global list
// if NOT found: return -1
int get_file_idx(const char *filename)
{
    int size;
    char **splitted;
    splitted = split(filename, &size);
    splitted[0] = "/";
    int parent_idx = search_dir_from_path_piece(splitted, size - 1, 0, 1);
    if (parent_idx < 0)
    {
        // errno = ENOENT;
        return -1;
    }; // parent path not exist
    Dir parent = dir_list[parent_idx];
    for (int i = 0; i < parent.children_size; ++i)
    {
        Map current_map = parent.directory_data[i];
        if (current_map.valid == 0 || current_map.type == 0)
            continue;
        if (strcmp(splitted[size - 1], file_list[current_map.target_idx].name) == 0)
            return current_map.target_idx;
    }
    // errno = ENOENT;
    return -1;
}

// get the index of a dir in the global list
// if NOT found: return -1
int get_dir_idx(const char *dirname)
{
    if (strcmp(dirname, "/") == 0)
        return 0;
    int size;
    char **splitted;
    splitted = split(dirname, &size);
    splitted[0] = "/";
    // printf("size: %d\n", size);
    int ret = search_dir_from_path_piece(splitted, size, 0, 1);
    // if (ret < 0) errno = ENOENT;
    return ret;
}

// init the root dir "/" in the global dir_list
void *my_init(struct fuse_conn_info *conn)
{
#ifdef DEBUG
    printf("[DEBUG] init\n");
    display_list();
#endif
    dir_idx++;
    strcpy(dir_list[dir_idx].name, "/");
    dir_list[dir_idx].children_size = 0;
#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return 0;
}

int my_getattr(const char *path, struct stat *st)
{
#ifdef DEBUG
    printf("[DEBUG] getattr: %s \n", path);
    display_list();
#endif

    st->st_uid = getuid();
    st->st_gid = getgid();

    if (get_dir_idx(path) >= 0)
    {
        st->st_mode = 0755 | S_IFDIR;
        st->st_nlink = 2;
    }
    else if (get_file_idx(path) >= 0)
    {
        if (file_list[get_file_idx(path)].is_symlink == 0)
        {
            st->st_mode = 0644 | S_IFREG;
            st->st_nlink = 1;
            // st->st_size = strlen(file_list[get_file_idx(path)].data);
            st->st_size = file_list[get_file_idx(path)].data_size;
        }
        else{
            st->st_mode = 0644 | S_IFLNK;
            st->st_nlink = 1;
            st->st_size = file_list[get_file_idx(path)].data_size;
        }
    }
    else
        return -ENOENT;
        // st->st_size = get_size(path);
#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return 0;
}

int my_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *)
{
#ifdef DEBUG
    printf("[DEBUG] READ\n");
    display_list();
#endif

    int target_idx = get_file_idx(path);
    if (target_idx == -1)
    {
        errno = ENOENT;
        return -1;
    }
    if (file_list[target_idx].is_symlink)
    {
        target_idx = get_file_idx(file_list[target_idx].data);
    }
    if (target_idx == -1)
    {
        errno = ENOENT;
        return -1;
    }
    char *target_data = file_list[target_idx].data;
    memcpy(buf, target_data + offset, size);

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif

    // return strlen(target_data) - offset;
    return size;
}

int my_write(const char *path, const char *content, size_t size, off_t offset,
             struct fuse_file_info *)
{
#ifdef DEBUG
    printf("[DEBUG] write: path %s, content: %s, size: %d, offset: %d\n", path, content, size, offset);
    display_list();
#endif

    int target_idx = get_file_idx(path);
    if (target_idx == -1)
    {
        errno = ENOENT;
        return -1;
    }
    if (file_list[target_idx].is_symlink)
    {
        target_idx = get_file_idx(file_list[target_idx].data);
    }
    if (target_idx == -1)
    {
        errno = ENOENT;
        return -1;
    }
    // file_list[target_idx].data_size = sizeof(content) / sizeof(char);
    // file_list[target_idx].data_size = offset + strlen(content);
    file_list[target_idx].data_size = offset + size;
    memcpy(file_list[target_idx].data + offset, content, size);

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif

    return size;
}

int my_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi)
{
#ifdef DEBUG
    printf("[DEBUG] readdir\n");
    display_list();
#endif

    int dir_idx = get_dir_idx(path);
    if (dir_idx < 0)
    {
        errno = ENOENT;
        return 1;
    }
    filler(buf, ".", NULL, 0);  // Current Directory
    filler(buf, "..", NULL, 0); // Parent Directory

    Dir root = dir_list[dir_idx];
    for (int i = 0; i < root.children_size; ++i)
    {
        Map cur = root.directory_data[i];
        int child_idx = cur.target_idx;
        if (cur.valid == 0)
            continue;
        if (cur.type == 0)
        {
            filler(buf, dir_list[child_idx].name, NULL, 0);
        }
        else
        {
            filler(buf, file_list[child_idx].name, NULL, 0);
        }
    }

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return 0;
}

int my_mkdir(const char *path, mode_t mode)
{
#ifdef DEBUG
    printf("[DEBUG] mkdir\n");
    display_list();
#endif

    int ret = add_dir(path);

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif

    return ret;
}

int my_mknod(const char *path, mode_t, dev_t)
{
#ifdef DEBUG
    printf("[DEBUG] mknod\n");
    display_list();
#endif

    // path++;
    int ret = add_file(path);

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return ret;
}

int my_open(const char *path, struct fuse_file_info *)
{
#ifdef DEBUG
    printf("[DEBUG] open\n");
    display_list();
#endif

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return 0;
}

int my_create(const char *, mode_t, struct fuse_file_info *)
{
#ifdef DEBUG
    printf("[DEBUG] create\n");
    display_list();
#endif

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return 0;
}

int my_readlink(const char * path, char * buf, size_t size)
{

#ifdef DEBUG
    printf("[DEBUG] readlink\n");
    display_list();
#endif
        int target_idx = get_file_idx(path);
    if (target_idx == -1)
    {
        errno = ENOENT;
        return -1;
    }
    if (!file_list[target_idx].is_symlink)
    {
        errno = EINVAL;
       return -1;
    }
    strcpy(buf,file_list[target_idx].data);
#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return 0;
}

int my_symlink(const char *target, const char *link_path)
{
#ifdef DEBUG
    printf("[DEBUG] symlink\n");
    display_list();
#endif
    // int from_idx = add_file(from);
    // int link_idx = add_file(link_path);
    int ret = add_file(link_path);
    if (ret < 0)
        return -1;
    int link_idx = get_file_idx(link_path);
    File *link_file = &file_list[link_idx];
    memcpy(link_file->data, target, strlen(target) + 1);
    link_file->is_symlink = 1;

#ifdef DEBUG
    printf("After: \n");
    display_list();
#endif
    return 0;
}