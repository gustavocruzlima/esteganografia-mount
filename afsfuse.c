
static int afs_getattr(const char *path, struct stat *stbuf) {
    int i;

    if (afs_dbg) { printf("getattr: %s\n", path); }
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    else {
        for (i = 0; i < afs->file_count; i++) {
            if (strcmp(path, afs->files[i]->name) == 0 && afs->files[i]->state != hole) {
                stbuf->st_uid = getuid();
                stbuf->st_gid = getgid();
                stbuf->st_mode = S_IFREG | 0644;
                stbuf->st_nlink = 1;
                stbuf->st_size = afs->files[i]->size;
                return 0;
            }
        }
    } 
    return -ENOENT;
}



static int afs_create(const char *path, mode_t mode, struct fuse_file_info *fi) {
    if (afs_dbg) { printf("create: %s\n", path); }
    return createFile((char *)path);
}



static int afs_open(const char *path, struct fuse_file_info *fi) {
    int f = findFile((char *)path);

    if (afs_dbg) { printf("abrir: %s\n", path); }
    if (f != -1) { return 0; }
    if ((fi->flags & 3) != O_RDONLY) { return -EACCES; }
    return -ENOENT;
}



static int afs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (afs_dbg) { printf("read: %s\n", path); }
    return readFile((char *)path, buf, size, offset);
}



static int afs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
    if (afs_dbg) { printf("write: %s : %d %d\n", path, (int)size, (int)offset); }
    return writeFile((char *)path, (char *)buf, size, offset);
}



static int afs_unlink(const char *path) {
    int f = findFile((char *)path);

    if (afs_dbg) { printf("desconectar: %s\n", path); }
    if (f == -1) { return -ENOENT; }
    wipeFile((char *)path);
    return 0;
}



static int afs_rename(const char *src, const char *dest) {
    int fsrc, fdest;

    if (afs_dbg) { printf("renomear: %s -> %s\n", src, dest); }
    if (strcmp(src, dest) == 0) { return 0; }
    fsrc = findFile((char *) src);
    if (fsrc == -1) { return -ENOENT; }
    fdest = findFile((char *) dest);
    if (fdest != -1) { 
        fprintf(stderr, "Não é possível renomear o arquivo\n");
        return -ENOENT;
    }
    strcpy(afs->files[fsrc]->name, dest);
    return 0;
}



static int afs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    int i;

    if (afs_dbg) { printf("readdir: %s\n", path); }
    if ((strcmp(path, "/")) != 0) { return -ENOENT; }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (i = 0; i < afs->file_count; i++) {
        if (afs->files[i]->state != hole)
            filler(buf, afs->files[i]->name + 1, NULL, 0);
    }
    return 0;
}



static int afs_utimens(const char *path, const struct timespec ts[2]) { return 0; }



static void afs_destroy(void *v) { saveState(); }


static struct fuse_operations afs_oper = {
    .getattr  = afs_getattr,
    .read     = afs_read,
    .readdir  = afs_readdir,
    .write    = afs_write,
    .open     = afs_open,
    .create   = afs_create,
    .rename   = afs_rename,
    .unlink   = afs_unlink,
    .utimens  = afs_utimens,
    .destroy  = afs_destroy,
};
