#ifndef TEK_H_
#define TEK_H_

#include <assert.h>
#include <stdlib.h>

#define TEK_REALLOC realloc
#define TEK_FREE free

// Slices can be any struct that has a pointer to items, length (int), and
// capacity (int)
#define TEK_SLICE_INIT_CAPACITY 256

#define tek_slice_reserve(slice, expected)                                     \
  do {                                                                         \
    if ((slice)->capacity == 0)                                                \
      (slice)->capacity = TEK_SLICE_INIT_CAPACITY;                             \
                                                                               \
    while ((expected) > (slice)->capacity)                                     \
      (slice)->capacity *= 2;                                                  \
                                                                               \
    (slice)->items = TEK_REALLOC((slice)->items,                               \
                                 sizeof(*(slice)->items) * (slice)->capacity); \
    assert((slice)->items != NULL && "Realloc failed");                        \
  } while (0)

#define tek_slice_append(slice, item)                                          \
  do {                                                                         \
    tek_slice_reserve((slice), (slice)->length + 1);                           \
    (slice)->items[(slice)->length++] = (item);                                \
  } while (0)

#define tek_slice_remove_unordered(slice, index)                               \
  do {                                                                         \
    size_t i = (index);                                                        \
    assert(i < (slice)->length && "Removal out of bounds");                    \
    (slice)->items[i] = (slice)->items[--(slice)->length];                     \
  } while (0)

#define tek_slice_free(slice)                                                  \
  do {                                                                         \
    TEK_FREE((slice)->items);                                                  \
    (slice)->items = NULL;                                                     \
    (slice)->length = 0;                                                       \
    (slice)->capacity = 0;                                                     \
  } while (0)

#endif // !TEK_H_
