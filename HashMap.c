#include "HashMap.h"

#define HASH_MAP_INITIAL_SIZE 0UL

/**
 * The function initializing all of the uninitialized vectors.
 * (so called buckets - I bet it makes you wanna get some nuggets).
 * @param hash_map a hash map.
 * @return 1 if allocation was done successfully, 0 otherwise.
 */
int InitializeVectors(HashMap *hash_map, size_t smaller_capacity) {
  for (size_t i = smaller_capacity; i < hash_map->capacity; ++i) {
    hash_map->buckets[i] = VectorAlloc(hash_map->pair_cpy, hash_map->pair_cmp, hash_map->pair_free);
    if (!hash_map->buckets[i]) return 0;
  }
  return 1;
}

/**
 * @return hash index.
 */
size_t ComputeHash(HashMap *hash_map, KeyT key) {
  return (((hash_map->hash_func(key)) & (hash_map->capacity - 1)));
}

HashMap *HashMapAlloc(
    HashFunc hash_func, HashMapPairCpy pair_cpy,
    HashMapPairCmp pair_cmp, HashMapPairFree pair_free) {
  if (!hash_func || !pair_cpy || !pair_cmp || !pair_free) return NULL;
  HashMap *hash_map = malloc(sizeof(HashMap));
  if (!hash_map) return NULL;
  hash_map->buckets = calloc(1, sizeof(Vector) * HASH_MAP_INITIAL_CAP);
  if (!hash_map->buckets) return NULL;
  hash_map->size = HASH_MAP_INITIAL_SIZE;    // number of Pairs in the hash map
  hash_map->capacity = HASH_MAP_INITIAL_CAP; // number of buckets/vectors
  hash_map->hash_func = hash_func;
  hash_map->pair_cpy = pair_cpy;
  hash_map->pair_cmp = pair_cmp;
  hash_map->pair_free = pair_free;
  if (!InitializeVectors(hash_map, HASH_MAP_INITIAL_SIZE)) return NULL;
  return hash_map;
}

void HashMapFree(HashMap **p_hash_map) {
  if (!p_hash_map) return;
  if (!(*p_hash_map)) {
    free(*p_hash_map);
    return;
  }
  if ((*p_hash_map)->buckets) {
    for (size_t i = 0; i < (*p_hash_map)->capacity; ++i) {
      VectorFree(&((*p_hash_map)->buckets[i]));
    }
    free((*p_hash_map)->buckets);
  }
  free(*p_hash_map);
}

/**
 * it resizes the hash map to the new given capacity.
 * @param hash_map a hash map.
 * @return returns 1 for successful insertion, 0 otherwise.
 */
HashMap *Rehash(HashMap *hash_map, size_t new_capacity) {
  // save the old capacity
  size_t old_capacity = hash_map->capacity;
  // update capacity
  hash_map->capacity = new_capacity;
  // handle case hash map will have more buckets now
  if (old_capacity < new_capacity) {
    Vector **temp_p_vector = realloc(hash_map->buckets, (hash_map->capacity) * (sizeof(Vector *)));
    if (!temp_p_vector) return NULL;
    hash_map->buckets = temp_p_vector;
    if (!InitializeVectors(hash_map, old_capacity)) return NULL;
  }
  // going over all pairs according to old capacity and putting in place according to new hash func result
  Vector *bucket = NULL, *bucket_to_insert = NULL;
  for (size_t current_bucket_index = 0; current_bucket_index < old_capacity; ++current_bucket_index) {
    bucket = (hash_map->buckets[current_bucket_index]);
    for (size_t current_pair_index = 0; current_pair_index < bucket->size; ++current_pair_index) {
      KeyT key = (*((Pair *) (bucket->data[current_pair_index]))).key;
      size_t bucket_to_insert_index = ComputeHash(hash_map, key);
      if (bucket_to_insert_index != current_bucket_index) {
        bucket_to_insert = hash_map->buckets[bucket_to_insert_index];
        VectorPushBack(bucket_to_insert, bucket->data[current_pair_index]);
        VectorErase(bucket, current_pair_index);
      }
    }
  }
  // handle case hash map will have less buckets now
  if (new_capacity < old_capacity) {
    // free all empty vectors we about to lose access to
    for (size_t i = hash_map->capacity; i < old_capacity; ++i) {
      VectorFree(&(hash_map->buckets[i]));
    }
    // reallocate buckets to new smaller size
    Vector **temp_p_vector = realloc(hash_map->buckets, (hash_map->capacity) * (sizeof(Vector *)));
    if (!temp_p_vector) return NULL;
    hash_map->buckets = temp_p_vector;
  }
  return hash_map;
}

int HashMapInsert(HashMap *hash_map, Pair *pair) {
  if (!hash_map || !pair) return 0;
  KeyT key = pair->key;
  // case key is not exist yet
  if (HashMapAt(hash_map, key) == NULL) {
    size_t index = ComputeHash(hash_map, key);
    Vector *bucket = hash_map->buckets[index];
    if (!VectorPushBack(bucket, pair)) return 0;
    ++hash_map->size;
  // case key is already exist (pair will be replace)
  } else {
    size_t index = ComputeHash(hash_map, key);
    Vector *bucket = hash_map->buckets[index];
    // find the right pair to replace
    for (size_t i = 0; i < bucket->size; ++i) {
      if (((Pair *) bucket->data[i])->key_cmp(((Pair *) bucket->data[i])->key, key)) {
        bucket->elem_free_func(&(bucket->data[i]));
        bucket->data[i] = hash_map->pair_cpy(pair);;
        break;
      }
    }
  }
  if (HashMapGetLoadFactor(hash_map) > HASH_MAP_MAX_LOAD_FACTOR) {
    hash_map = Rehash(hash_map, (hash_map->capacity) * VECTOR_GROWTH_FACTOR);
    if (!hash_map) return 0;
  }
  return 1;
}

int HashMapContainsKey(HashMap *hash_map, KeyT key) {
  if (!hash_map || !key) return 0;
  size_t index = ComputeHash(hash_map, key);
  Vector *bucket = hash_map->buckets[index];
  // figure out if key already in the hash map
  for (size_t i = 0; i < bucket->size; ++i) {
    if (((Pair *) bucket->data[i])->key_cmp(((Pair *) bucket->data[i])->key, key)) {
      return 1;
    }
  }
  return 0;
}

int HashMapContainsValue(HashMap *hash_map, ValueT value) {
  if (!hash_map || !value) return 0;
  size_t pairs_already_checked = 0;
  size_t current_bucket = 0;
  // check for every pair in the hash map if value is the wanted one
  while (pairs_already_checked < hash_map->size) {
    Vector *bucket = hash_map->buckets[current_bucket];
    for (size_t i = 0; i < bucket->size; ++i) {
      if (((Pair *) bucket->data[i])->value_cmp(((Pair *) bucket->data[i])->value, value)) return 1;
      ++pairs_already_checked;
    }
    ++current_bucket;
  }
  return 0;
}

ValueT HashMapAt(HashMap *hash_map, KeyT key) {
  if (!hash_map || !key) return NULL;
  size_t index = ComputeHash(hash_map, key);
  Vector *bucket = hash_map->buckets[index];
  for (size_t i = 0; i < bucket->size; ++i) {
    if (((Pair *) bucket->data[i])->key_cmp(((Pair *) bucket->data[i])->key, key)) {
      return ((Pair *) bucket->data[i])->value;
    }
  }
  return NULL;
}

int HashMapErase(HashMap *hash_map, KeyT key) {
  if (!hash_map || !key) return 0;
  size_t index = ComputeHash(hash_map, key);
  Vector *bucket = hash_map->buckets[index];
  for (size_t i = 0; i < bucket->size; ++i) {
    if (((Pair *) (bucket->data[i]))->key_cmp(((Pair *) (bucket->data[i]))->key, key)) {
      VectorErase(bucket, i);
      --hash_map->size;
      if (HashMapGetLoadFactor(hash_map) < HASH_MAP_MIN_LOAD_FACTOR) {
        if (!Rehash(hash_map, hash_map->capacity / VECTOR_GROWTH_FACTOR)) return 0;
      }
      return 1;
    }
  }
  return 0;
}

double HashMapGetLoadFactor(HashMap *hash_map) {
  if (!hash_map) return -1;
  return (double) ((double) hash_map->size / hash_map->capacity);
}

void HashMapClear(HashMap *hash_map) {
  if (!hash_map) return;
  Vector *bucket = NULL;
  while (hash_map->size) {
    for (size_t i = 0; i < hash_map->capacity; ++i) { // going over vectors
      bucket = hash_map->buckets[i];
      if (hash_map->size) {
        hash_map->size = hash_map->size - bucket->size;
        VectorClear(bucket);
        if (HashMapGetLoadFactor(hash_map) < HASH_MAP_MIN_LOAD_FACTOR) {
          Rehash(hash_map, hash_map->capacity / VECTOR_GROWTH_FACTOR);
        }
      }
    }
  }
}
