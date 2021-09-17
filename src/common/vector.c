#include "common/vector.h"

extern int my_errno;

struct vector vector_create(size_t element_size) {
  struct vector result = {.num = 0, .max_num = 8, .el_size = element_size, 
                          .data = NULL};
  void *temp = calloc(result.max_num, result.el_size);
  if (temp == NULL) {
    my_errno = errno;
    result.max_num = 0;
    return result;
  }
  result.data = temp;
  return result;
}

void vector_pushback(struct vector *vec, void *arg) {
  assert(vec != NULL);
  if (vec->num >= vec->max_num) {
    void *temp = realloc(vec->data, 2 * vec->max_num * vec->el_size);
    if (temp == NULL) {
      my_errno = errno;
      return;
    }
    vec->data = temp;
    vec->max_num *= 2;
  }
  (void) memcpy(vector_get(vec, vec->num), arg, vec->el_size);
  ++vec->num;
}

void vector_clear(struct vector *vec, void (*dest)(void *)) {
  assert(vec != NULL);
  if (dest != NULL) {
    for (size_t i = 0; i < vec->num; ++i) {
      dest(vector_get(vec, i));
    }
  }
  vec->num = 0;
}

void *vector_get(struct vector *vec, size_t n) {
  assert(vec != NULL);
  return vec->data + n * vec->el_size;
}

void vector_destroy(struct vector *vec, void (*dest)(void *)) {
  assert(vec != NULL);
  vector_clear(vec, dest);
  free(vec->data);
  vec->data = NULL;
}

size_t vector_size(struct vector *vec) {
  assert(vec != NULL);
  return vec->num;
}

