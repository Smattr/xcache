#include "log.h"
#include <stdio.h>
#include <stdlib.h>

verbosity_t verbosity = L_ERROR;

FILE *log_file;

int log_initialised;

int log_init(const char *filename) {
    if (filename == NULL) {
        log_file = stderr;
    } else {
        log_file = fopen(filename, "a");
        if (log_file == NULL) {
            return -1;
        }
    }
    log_initialised = 1;
    return 0;
}

void log_deinit(void) {
    if (log_file != stderr) {
        fclose(log_file);
    }
    log_initialised = 0;
}
