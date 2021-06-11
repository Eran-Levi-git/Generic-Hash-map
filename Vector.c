#include "Vector.h"
#include <stdio.h>

#define VECTOR_EMPTY_SIZE 0UL

Vector *VectorAlloc(VectorElemCpy elem_copy_func, VectorElemCmp elem_cmp_func, VectorElemFree elem_free_func) {
  if (!elem_copy_func || !elem_cmp_func || !elem_free_func) return NULL;
  Vector *vector = malloc(sizeof(Vector));
  if (!vector) return NULL;
  vector->data = (void *) malloc(sizeof(void *) * VECTOR_INITIAL_CAP);
  if (!vector->data) return NULL;
  vector->capacity = VECTOR_INITIAL_CAP;
  vector->size = VECTOR_EMPTY_SIZE;
  vector->elem_copy_func = elem_copy_func;
  vector->elem_cmp_func = elem_cmp_func;
  vector->elem_free_func = elem_free_func;
  return vector;
}

void VectorFree(Vector **p_vector) {
  if (!p_vector) return;
  if (!*p_vector) {
    free((*p_vector));
    return;
  }
  for (size_t i = 0; i < (*p_vector)->size; ++i) {
    (*p_vector)->elem_free_func(&(*p_vector)->data[i]);
  }
  free((*p_vector)->data);
  free((*p_vector));
}

void *VectorAt(Vector *vector, size_t ind) {
  if (!vector) return NULL;
  if (vector->size <= ind) return NULL;
  return vector->data[ind];
}

int VectorFind(Vector *vector, void *value) {
  if (!vector || !value) return -1;
  for (size_t i = 0; i < vector->size; ++i) {
    if (vector->elem_cmp_func(vector->data[i], value)) return i;
  }
  return -1;
}

int VectorPushBack(Vector *vector, void *value) {
  if (!vector || !value) return 0;
  vector->data[vector->size] = vector->elem_copy_func(value);
  ++vector->size;
  if (VECTOR_MAX_LOAD_FACTOR < VectorGetLoadFactor(vector)) {
    void *tmp = realloc(vector->data, sizeof(Vector) * vector->capacity * VECTOR_GROWTH_FACTOR);
    if (!tmp) {
      free(tmp);
      return 0;
    }
    vector->data = tmp;
    vector->capacity = vector->capacity * VECTOR_GROWTH_FACTOR;
  }
  return 1;
}

double VectorGetLoadFactor(Vector *vector) {
  if (!vector) return -1;
  return (double) (((double) (vector->size)) / vector->capacity);
}

int VectorErase(Vector *vector, size_t ind) {
  if (!vector || (vector->size <= ind)) return 0;
  vector->elem_free_func(&vector->data[ind]);
  while (ind < (vector->size - 1)) {
    vector->data[ind] = vector->data[ind + 1];
    ++ind;
  }
  --(vector->size);
  if (VectorGetLoadFactor(vector) < VECTOR_MIN_LOAD_FACTOR) {
    void *tmp = realloc(vector->data, sizeof(void *) * (vector->capacity / VECTOR_GROWTH_FACTOR));
    if (!tmp) return 0;
    vector->data = tmp;
    vector->capacity = (vector->capacity / VECTOR_GROWTH_FACTOR);
  }
  return 1;
}

void VectorClear(Vector *vector) {
  if (!vector) return;
  size_t i = vector->size;
  while (i != VECTOR_EMPTY_SIZE) {
    VectorErase(vector, i);
    --i;
  }
  VectorErase(vector, i);
}
