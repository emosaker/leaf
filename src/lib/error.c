/*
 * This file is part of the leaf programming language
 */

#include <stdio.h>

#include "lib/error.h"
#include "lib/ansi.h"

void error_underline_code(const char *source, int idx_start, int idx_end) {
    /* find start of the line with the error */
    int line_start = idx_start;
    while (line_start > 0 && source[line_start] && source[line_start] != '\n') {
        line_start -= 1;
    }
    if (source[line_start] == '\n' && source[line_start + 1]) {
        line_start += 1;
    }

    int i = line_start;
    int j = line_start;
    while (i < idx_end) {
        /* print the line of code */
        while (source[i] && source[i] != '\n') {
            putc(source[i], stdout);
            i += 1;
        }
        putc('\n', stdout);
        i += 1;

        /* add underlines */
        printf(FG_RED BOLD);
        while (j < idx_start) {
            putc(' ', stdout);
            j += 1;
        }
        if (j == idx_start) {
            putc('^', stdout);
            j += 1;
        }
        printf(CROSSED);
        while (source[j] && source[j] != '\n' && j < idx_end) {
            putc('~', stdout);
            j += 1;
        }
        printf(RESET);
        putc('\n', stdout);
        j += 1;
    }
}

void lf_error_print(const char *file, const char *source, int idx_start, int idx_end, const char *message) {
    int line_start = idx_start;
    while (line_start > 0 && source[line_start] && source[line_start] != '\n') {
        line_start -= 1;
    }

    int column = idx_start - line_start + 1 - (source[line_start] == '\n' ? 1 : 0);
    int line = 1;
    while (line_start > 0 && source[line_start]) {
        if (source[line_start] == '\n') {
            line += 1;
        }
        line_start -= 1;
    }

    printf("%s:%d:%d: %s:\n", file, line, column, message);
    error_underline_code(source, idx_start, idx_end);
}
