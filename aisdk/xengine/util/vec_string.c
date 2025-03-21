#include "vec_string.h"
void vector_string_init(VectorString *vector) {
    vector->data = NULL;
    vector->size = 0;
    vector->capacity = 0;
}

void vector_string_free(VectorString *vector) {
    for (size_t i = 0; i < vector->size; ++i) {
        free(vector->data[i]);
    }
    free(vector->data);
}

void vector_string_push(VectorString *vector, const char *str) {
    if (vector->size >= vector->capacity) {
        vector->capacity = (vector->capacity == 0) ? 10 : vector->capacity * 2;
        vector->data = realloc(vector->data, vector->capacity * sizeof(char *));
    }
    vector->data[vector->size] = strdup(str);
    vector->size++;
}
int has_file_extension(const char *filename, const char *extension) {
    // 找到最后一个点（.）的位置
    const char *dot = strrchr(filename, '.');

    // 如果没有点，返回0（没有后缀）
    if (!dot) {
        return 0;
    }

    // 获取后缀字符串
    const char *file_extension = dot + 1;

    // 比较后缀
    return strcmp(file_extension, extension) == 0;
}
