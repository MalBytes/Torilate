/* 
    File: src/util/file.c
    Author: Trident Apollo
    Date: 6-02-2026
    Reference:
        - cppreference: https://en.cppreference.com/w/c/io.html
        - GFG: https://www.geeksforgeeks.org/c/basics-file-handling-c/
    Description:
        Utility wrapper functions for file handling operations. Provides a simple interface for 
        common file operations such as reading from and writing to files, while ensuring proper error 
        handling.
*/

#include "util/util.h"


Error write_to(const char *file_name, const char *data, size_t len) {
    Error err = ERR_OK();
    FILE *file = fopen(file_name, "wb");
    if (!file) {
        int err = errno;
        switch (err) {
            case ENOENT:    return ERR_NEW(ERR_FILE_NOT_FOUND, "File '%s' not found", file_name);
            case EACCES:    return ERR_NEW(ERR_NO_PERMISSION, "No permission to write to file '%s'", file_name);
            case ENOMEM:    return ERR_NEW(ERR_OUTOFMEMORY, "Out of memory while writing to file '%s'", file_name);
            default:        return ERR_NEW(ERR_IO, "Failed to open file '%s' for writing", file_name); 
        }
    }

    size_t written = fwrite(data, 1, len, file);
    int flush_status = fflush(file);
    if (written != len || flush_status != 0) {
        err = ERR_NEW(ERR_IO, "Failed to write to file '%s'", file_name);
        goto exit_write;
    }

exit_write:
    fclose(file);
    return err;
}

Error read_from(const char *file_name, char **buffer, size_t *out_len) {
    Error err = ERR_OK();
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        int err = errno;
        switch (err) {
            case ENOENT:    return ERR_NEW(ERR_FILE_NOT_FOUND, "File '%s' not found", file_name);
            case EACCES:    return ERR_NEW(ERR_NO_PERMISSION, "No permission to read file '%s'", file_name);
            case ENOMEM:    return ERR_NEW(ERR_OUTOFMEMORY, "Out of memory while reading from file '%s'", file_name);
            default:        return ERR_NEW(ERR_IO, "Failed to open file '%s' for reading", file_name);
        }
    }

    if (fseek(file, 0, SEEK_END) != 0) {
        err = ERR_NEW(ERR_IO, "Failed to seek end of file '%s'", file_name);
        goto exit_read;
    }
    long size = ftell(file);
    if (size < 0) {
        err = ERR_NEW(ERR_IO, "Failed to determine size of file '%s'", file_name);
        goto exit_read;
    }
    rewind(file);

    char *data = (char *)malloc(size + 1);
    if (!data) {
        err = ERR_NEW(ERR_OUTOFMEMORY, "Out of memory while allocating buffer for file '%s'", file_name);
        goto exit_read;
    }
    size_t read_size = fread(data, 1, size, file);
    if (read_size != (size_t)size) {
        err = ERR_NEW(ERR_IO, "Failed to read file '%s'", file_name);
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

    return err;
}
