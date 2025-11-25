#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "utils.h"

void trim_newline(char *s){
    if (!s) return;
    int L = strlen(s);
    while (L>0 && (s[L-1]=='\n' || s[L-1]=='\r')) { s[--L] = 0; }
}

int file_exists(const char *path){
    struct stat st;
    return stat(path, &st) == 0;
}
