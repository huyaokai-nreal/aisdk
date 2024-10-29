#pragma  once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char **data;
    size_t size;
    size_t capacity;
} VectorString;
int has_file_extension(const char *filename, const char *extension);
void vector_string_init(VectorString *vector);
void vector_string_push(VectorString *vector, const char *str);
void vector_string_free(VectorString *vector);

