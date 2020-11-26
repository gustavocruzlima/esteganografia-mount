#include "include/options.h"
#include "include/libpng.h"

afs_filesystem *afs;

#include "afs.c"
#include "afsfuse.c"

int main(int argc, char *argv[]) {
    char actualpath [MAX_PATH];
    char* base;
    int8_t fargc = 6;
    const char *fargv[fargc];
    char key_input[4096];

    if ((getuid() == 0) || (geteuid() == 0)) {
        fprintf(stderr, "Não roda como root!!\n");
        exit(1);
    }
    if (argc < 3) { afs_usage(); }

    afs_dbg = 0;
    if(parseArgv(argc, argv, DEBUG_OPTION)) { afs_dbg = 1; }
    if(parseArgv(argc, argv, HELP_OPTION)) { afs_usage(); }

    realpath(argv[argc - 1], actualpath);
    base = basename(actualpath);
    afs = malloc(sizeof(afs_filesystem));
    afs->root_img = malloc(sizeof(png_data));
    strncpy(afs->img_dir, actualpath, strlen(actualpath) - strlen(base));
    strcpy(afs->root_img->filename, base);

    if (!read_png(afs->root_img, afs->img_dir)) { afs_usage(); }

    printf("Entre o nome do drive:\n");
    fgets(afs->name, sizeof(afs->name), stdin);
    afs->name[strlen(afs->name) - 1] = '\0';
    strcat(afs->name, ".afs");

    printf("Entre a chave de criptografia %s:\n", afs->name);
    fgets(key_input, sizeof(key_input), stdin);
    SHA512_CTX sha512;
    SHA512_Init(&sha512);
    SHA512_Update(&sha512, key_input, strlen(key_input));
    SHA512_Final((unsigned char *)afs->key, &sha512);
    printf("\e[1;1H\e[2J");


    if (parseArgv(argc, argv, MNT_OPTION)) { readRoot(); }

    else if (parseArgv(argc, argv, FORMAT_OPTION)) { afs_format(); }

    else if (parseArgv(argc, argv, EXPAND_OPTION)) { afs_expand(); }
    else { afs_usage(); }


    fargv[0] = argv[0];
    fargv[1] = "-f";
    fargv[2] = "-o";
    fargv[3] = "big_writes";
    fargv[4] = "-s";
    fargv[5] = afs->name;
    mkdir(afs->name, 0755);
    printf("\n%s montado.\nUse Ctrl+C para desmontar de modo seguro.\n", afs->name);
    return fuse_main(fargc, (char **)fargv, &afs_oper, NULL);
}



void afs_usage() {
    fprintf(stderr, "main [-h (help)] [-d (debug)] [-e (expand)] [-f (format)] [-m (mount)] <caminho/para/imagem.png>\n");
    exit(1);
}



int parseArgv(int argc, char *argv[], char *option) {
    int8_t length = strlen(option);
    int8_t y;
    for(y = 0; y < argc; y++) {
        if(strncmp(argv[y], option, length) == 0) { return y; }
    }
    return 0;
}


int getMD5(char *filename, char *md5_sum) {
    char path[MAX_PATH];
    strcpy(path, afs->img_dir);
    strcat(path, filename);
    FILE *f = fopen(path, "rb");
    char data[sizeof(md5_sum)];

    if (f == NULL) { return 0; }
    MD5_CTX mdContext;
    MD5_Init (&mdContext);
	while (fread (data, 1, sizeof(md5_sum), f) != 0) {
            MD5_Update (&mdContext, data, sizeof(md5_sum));
        }
    MD5_Final ((unsigned char *)md5_sum, &mdContext);
    pclose(f);
    return 1;
}
