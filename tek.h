#ifndef TEK_H_
#define TEK_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef TEK_ASSERT
#include <assert.h>
#define TEK_ASSERT assert
#endif

#if defined(TEK_MALLOC) && defined(TEK_FREE) && defined(TEK_REALLOC)
#elif !defined(TEK_MALLOC) && !defined(TEK_FREE) && !defined(TEK_REALLOC)
#else
#error "Must define all or none of TEK_MALLOC, TEK_FREE, and TEK_REALLOC."
#endif
#ifndef TEK_MALLOC
#include <stdlib.h>
#define TEK_MALLOC malloc
#define TEK_REALLOC realloc
#define TEK_FREE free
#endif

// Slices can be any struct that has a pointer to items, length (size_t), and
// capacity (size_t)
#define TEK_SLICE_INIT_CAPACITY 64

#define tek_slice_reserve(slice, expected)                                                                                                 \
  do {                                                                                                                                     \
    if ((slice)->capacity == 0) (slice)->capacity = TEK_SLICE_INIT_CAPACITY;                                                               \
    while ((expected) > (slice)->capacity) (slice)->capacity *= 2;                                                                         \
    (slice)->items = TEK_REALLOC((slice)->items, sizeof(*(slice)->items) * (slice)->capacity);                                             \
    TEK_ASSERT((slice)->items != NULL && "Realloc failed");                                                                                \
  } while (0)

#define tek_slice_append(slice, item)                                                                                                      \
  do {                                                                                                                                     \
    tek_slice_reserve((slice), (slice)->length + 1);                                                                                       \
    (slice)->items[(slice)->length++] = (item);                                                                                            \
  } while (0)

#define tek_slice_remove_unordered(slice, index)                                                                                           \
  do {                                                                                                                                     \
    size_t i = (index);                                                                                                                    \
    TEK_ASSERT(i < (slice)->length && "Removal out of bounds");                                                                            \
    (slice)->items[i] = (slice)->items[--(slice)->length];                                                                                 \
  } while (0)

#define tek_slice_free(slice)                                                                                                              \
  do {                                                                                                                                     \
    if ((slice)->items) TEK_FREE((slice)->items);                                                                                          \
    (slice)->items = NULL;                                                                                                                 \
    (slice)->length = 0;                                                                                                                   \
    (slice)->capacity = 0;                                                                                                                 \
  } while (0)

#endif // !TEK_H_
