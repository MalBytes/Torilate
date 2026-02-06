/* 
    File: src/util/file.c
    Author: Trident Apollo
    Date: 23-01-2026
    Reference:
        - cppreference: https://en.cppreference.com/w/c/io.html
        - GFG: https://www.geeksforgeeks.org/c/basics-file-handling-c/
    Description:
        Utility wrapper functions for file handling operations. Provides a simple interface for 
        common file operations such as reading from and writing to files, while ensuring proper error 
        handling.
*/

#include <errno.h>
#include "util/util.h"


int write_to(const char *file_name, const char *data, size_t len) {
    int return_code = SUCCESS;
    FILE *file = fopen(file_name, "wb");
    if (!file) {
        int err = errno;
        switch (err) {
            case ENOENT:    return ERR_FILE_NOT_FOUND;
            case EACCES:    return ERR_NO_PERMISSION;
            case ENOMEM:    return ERR_OUTOFMEMORY;
            default:        return ERR_IO; 
        }
    }

    size_t written = fwrite(data, 1, len, file);
    int flush_status = fflush(file);
    if (written != len || flush_status != 0) {
        return_code = ERR_IO;
        goto exit_write;
    }

exit_write:
    fclose(file);

    return return_code;
}

int read_from(const char *file_name, char **buffer, size_t *out_len) {
    int return_code = SUCCESS;
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        int err = errno;
        switch (err) {
            case ENOENT:    return ERR_FILE_NOT_FOUND;
            case EACCES:    return ERR_NO_PERMISSION;
            case ENOMEM:    return ERR_OUTOFMEMORY;
            default:        return ERR_IO; 
        }
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        return_code = ERR_IO;
        goto exit_read;
    }
    long size = ftell(file);
    if (size < 0) {
        return_code = ERR_IO;
        goto exit_read;
    }
    rewind(file);

    char *data = (char *)malloc(size + 1);
    if (!data) {
        return_code = ERR_OUTOFMEMORY;
        goto exit_read;
    }
    size_t read_size = fread(data, 1, size, file);
    if (read_size != (size_t)size) {
        return_code = ERR_IO;
        goto exit_read;
    }
    data[size] = '\0';

    *buffer = data;
    if (out_len) { *out_len = size; }
    data = NULL;

exit_read:
    if (data) {
        free(data);
    }
    fclose(file);

    return return_code;
}
