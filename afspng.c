#include <stdio.h>
#include <stdlib.h>
#include <png.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <libgen.h>
#include "include/libpng.h"

int read_png(png_data *img, char *img_dir) {
    int y;
    char fullpath [PATH_MAX];

    sprintf(fullpath, "%s%s", img_dir, img->filename);


    FILE *fp = fopen(fullpath, "r+");
    if (!fp) {
        fprintf(stderr, "Não é possível abrir %s\n", img->filename);
        free(img);
        return 0;
    }

    fread(img->png_sig, 1, 8, fp);
    if (png_sig_cmp(img->png_sig, 0, 8)) {
        fprintf(stderr, "%s assinatura inválida\n", img->filename);
        fclose(fp);
        free(img);
        return 0;
    }

    img->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!img->png_ptr) {
        fprintf(stderr, "sem memória\n");
        fclose(fp);
        free(img);
        return 0;
    }

    img->info_ptr = png_create_info_struct(img->png_ptr);
    if (!img->info_ptr) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fprintf(stderr, "sem memória\n");
        fclose(fp);
        free(img);
        return 0;
    }

    if (setjmp(png_jmpbuf(img->png_ptr))) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fclose(fp);
        free(img);
        return 0;
    }

    png_init_io(img->png_ptr, fp);
    png_set_sig_bytes(img->png_ptr, 8);
    png_read_info(img->png_ptr, img->info_ptr);

    img->width = png_get_image_width(img->png_ptr, img->info_ptr);
    img->height = png_get_image_height(img->png_ptr, img->info_ptr);
    img->bit_depth = png_get_bit_depth(img->png_ptr, img->info_ptr);
    img->color_type = png_get_color_type(img->png_ptr, img->info_ptr);
    img->channels = png_get_channels(img->png_ptr, img->info_ptr);
    img->state = not_modified;

    if (img->color_type != PNG_COLOR_TYPE_RGB && img->color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
        fprintf(stderr, "A imagem deve ser RGB\n");
        free(img);
        return 0;
    }

    if (img->bit_depth == 16) { png_set_strip_16(img->png_ptr); }

    if (img->color_type & PNG_COLOR_MASK_ALPHA) { png_set_strip_alpha(img->png_ptr); }
    png_read_update_info(img->png_ptr, img->info_ptr);


    img->row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * img->height);
    for(y = 0; y < img->height; y++) {
        img->row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(img->png_ptr, img->info_ptr));
    }
    png_read_image(img->png_ptr, img->row_pointers);
    fclose(fp);
    return 1;
}


int write_png(png_data *img, char *img_dir) {
    char fullpath [PATH_MAX];

    sprintf(fullpath, "%s%s", img_dir, img->filename);
    FILE *fp = fopen(fullpath, "wb");
    if (!fp) {
        fprintf(stderr, "Falha na escrita %s! Impossível a escrita\n", img->filename);
        free(img);
        return 0;
    }
    img->png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!img->png_ptr) {
        fprintf(stderr, "Falha na escrita %s! Sem memória\n", img->filename);
        fclose(fp);
        free(img);
        return 0;
    }
    img->info_ptr = png_create_info_struct(img->png_ptr);
    if (!img->info_ptr) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fprintf(stderr, "Falha na escrita %s! Sem memória\n", img->filename);
        fclose(fp);
        free(img);
        return 0;
    }
    if (setjmp(png_jmpbuf(img->png_ptr))) {
        png_destroy_read_struct(&img->png_ptr, NULL, NULL);
        fclose(fp);
        free(img);
        return 0;
    }
    png_init_io(img->png_ptr, fp);

    png_set_IHDR(
        img->png_ptr,
        img->info_ptr,
        img->width, img->height,
        8,
        PNG_COLOR_TYPE_RGB,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
        PNG_FILTER_TYPE_BASE
    );
    png_write_info(img->png_ptr, img->info_ptr);
    png_write_image(img->png_ptr, img->row_pointers);
    png_write_end(img->png_ptr, NULL);
    fclose(fp);
    return 1;
}
