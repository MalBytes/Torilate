/*
    File: src/util/memory.c
    Author: Trident Apollo  
    Date: 09-02-2026
    Reference: None
    Description:
        Memory utility function implementations for Torilate.
*/

#include "util/util.h"

void cleanup_uri(URI *uri) {
    if (uri->host) {
        free((void*) uri->host);
        uri->host = NULL;
    }
    if (uri->path) {
        free((void*) uri->path);
        uri->path = NULL;
    }

    memset(uri, 0, sizeof(URI));
}

char *ut_strdup(const char *s) {
    size_t size = strlen(s) + 1;
    char *p = malloc(size);
    if (p != NULL) {
        memcpy(p, s, size);
    }
    p[size - 1] = '\0';
    return p;
}

char *ut_strndup(const char *s, size_t n) {
    char *p;
    size_t n1;

    for (n1 = 0; n1 < n && s[n1] != '\0'; n1++)
        continue;
    p = malloc(n + 1);
    if (p != NULL) {
        memcpy(p, s, n1);
        p[n1] = '\0';
    }
    return p;
}