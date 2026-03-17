#ifndef TEK_H_
#define TEK_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef TEK_ASSERT
#include <assert.h>
#define TEK_ASSERT assert
#endif

#ifndef TEK_MALLOC
#include <stdlib.h>
#define TEK_MALLOC malloc
#endif

#ifndef TEK_REALLOC
#include <stdlib.h>
#define TEK_REALLOC realloc
#endif

#ifndef TEK_FREE
#include <stdlib.h>
#define TEK_FREE free
#endif

// Slices can be any struct that has a pointer to items, length (size_t), and
// capacity (size_t)
#define TEK_SLICE_INIT_CAPACITY 64

#define tek_slice_reserve(slice, expected)                                     \
  do {                                                                         \
    if ((slice)->capacity == 0)                                                \
      (slice)->capacity = TEK_SLICE_INIT_CAPACITY;                             \
    while ((expected) > (slice)->capacity)                                     \
      (slice)->capacity *= 2;                                                  \
    (slice)->items = TEK_REALLOC((slice)->items,                               \
                                 sizeof(*(slice)->items) * (slice)->capacity); \
    TEK_ASSERT((slice)->items != NULL && "Realloc failed");                    \
  } while (0)

#define tek_slice_append(slice, item)                                          \
  do {                                                                         \
    tek_slice_reserve((slice), (slice)->length + 1);                           \
    (slice)->items[(slice)->length++] = (item);                                \
  } while (0)

#define tek_slice_remove_unordered(slice, index)                               \
  do {                                                                         \
    size_t i = (index);                                                        \
    TEK_ASSERT(i < (slice)->length && "Removal out of bounds");                \
    (slice)->items[i] = (slice)->items[--(slice)->length];                     \
  } while (0)

#define tek_slice_free(slice)                                                  \
  do {                                                                         \
    TEK_FREE((slice)->items);                                                  \
    (slice)->items = NULL;                                                     \
    (slice)->length = 0;                                                       \
    (slice)->capacity = 0;                                                     \
  } while (0)

typedef enum {
  Bool = 0,
  Int,
  Float,
  Double,
  Cstr,
} TekFlagKind;

typedef union {
  bool as_bool;
  uint64_t as_int;
  float as_float;
  double as_double;
  char *as_cstr;
} TekFlagValue;

typedef struct TekFlag TekFlag;
struct TekFlag {
  TekFlagKind kind;
  TekFlagValue value;
  TekFlagValue _default;
  void *ref;
  const char *name;
  const char *help;
};

typedef struct TekFlagContext TekFlagContext;
struct TekFlagContext {
  TekFlag *items;
  size_t length, capacity;
};

bool *tek_flag_bool(const char *name, const char *help, bool _default);
bool *tek_flag_ctx_bool(TekFlagContext *ctx, const char *name, const char *help, bool _default);
void tek_flag_bool_var(const char *name, const char *help, bool *variable, bool _default);
void tek_flag_ctx_bool_var(TekFlagContext *ctx, const char *name, const char *help, bool *variable, bool _default);

uint64_t *tek_flag_int(const char *name, const char *help, uint64_t _default);
uint64_t *tek_flag_ctx_int(TekFlagContext *ctx, const char *name, const char *help, uint64_t _default);
void tek_flag_int_var(const char *name, const char *help, uint64_t *variable, uint64_t _default);
void tek_flag_ctx_int_var(TekFlagContext *ctx, const char *name, const char *help, uint64_t *variable, uint64_t _default);

float *tek_flag_float(const char *name, const char *help, float _default);
float *tek_flag_ctx_float(TekFlagContext *ctx, const char *name, const char *help, float _default);
void tek_flag_float_var(const char *name, const char *help, float *variable, float _default);
void tek_flag_ctx_float_var(TekFlagContext *ctx, const char *name, const char *help, float *variable, float _default);

double *tek_flag_double(const char *name, const char *help, double _default);
double *tek_flag_ctx_double(TekFlagContext *ctx, const char *name, const char *help, double _default);
void tek_flag_double_var(const char *name, const char *help, double *variable, double _default);
void tek_flag_ctx_double_var(TekFlagContext *ctx, const char *name, const char *help, double *variable, double _default);

char **tek_flag_cstr(const char *name, const char *help, const char *_default);
char **tek_flag_ctx_cstr(TekFlagContext *ctx, const char *name, const char *help, const char *_default);
void tek_flag_cstr_var(const char *name, const char *help, char **variable, const char *_default);
void tek_flag_ctx_cstr_var(TekFlagContext *ctx, const char *name, const char *help, char **variable, const char *_default);

bool tek_flag_parse(int argc, char *argv[]);
bool tek_flag_parse_ctx(TekFlagContext *ctx, int argc, char *argv[]);

void tek_flag_print();
void tek_flag_print_ctx(TekFlagContext *ctx);

#define TEK_IMPLEMENTATION // TODO: Remove this line
#ifdef TEK_IMPLEMENTATION
#include <stdio.h>
#include <string.h>

static TekFlagContext global_flag_context;

static TekFlag *tek_new_flag(TekFlagContext *ctx, TekFlagKind kind, const char *name, const char *help) {
  TekFlag flag = {0};
  tek_slice_append(ctx, flag);
  TekFlag* f = &ctx->items[ctx->length-1];
  f->kind = kind;
  f->name = name;
  f->help = help;
  return f;
}

bool *tek_flag_bool(const char *name, const char *help, bool _default) { return tek_flag_ctx_bool(&global_flag_context, name, help, _default); }
bool *tek_flag_ctx_bool(TekFlagContext *ctx, const char *name, const char *help, bool _default)
{
  TekFlag *f = tek_new_flag(ctx, Bool, name, help);
  f->ref = &f->value.as_bool;
  *(bool*)f->ref = _default;
  f->_default.as_bool = _default;
  return f->ref;
}
void tek_flag_bool_var(const char *name, const char *help, bool *variable, bool _default) { tek_flag_ctx_bool_var(&global_flag_context, name, help, variable, _default); }
void tek_flag_ctx_bool_var(TekFlagContext *ctx, const char *name, const char *help, bool *variable, bool _default)
{
  TekFlag *f = tek_new_flag(ctx, Bool, name, help);
  f->ref = variable;
  *(bool*)f->ref = _default;
  f->_default.as_bool = _default;
}

uint64_t *tek_flag_int(const char *name, const char *help, uint64_t _default) { return tek_flag_ctx_int(&global_flag_context, name, help, _default); }
uint64_t *tek_flag_ctx_int(TekFlagContext *ctx, const char *name, const char *help, uint64_t _default)
{
  TekFlag *f = tek_new_flag(ctx, Int, name, help);
  f->ref = &f->value.as_int;
  *(uint64_t*)f->ref = _default;
  f->_default.as_int = _default;
  return f->ref;
}
void tek_flag_int_var(const char *name, const char *help, uint64_t *variable, uint64_t _default) { tek_flag_ctx_int_var(&global_flag_context, name, help, variable, _default); }
void tek_flag_ctx_int_var(TekFlagContext *ctx, const char *name, const char *help, uint64_t *variable, uint64_t _default)
{
  TekFlag *f = tek_new_flag(ctx, Int, name, help);
  f->ref = variable;
  *(uint64_t*)f->ref = _default;
  f->_default.as_int = _default;
}

float *tek_flag_float(const char *name, const char *help, float _default) { return tek_flag_ctx_float(&global_flag_context, name, help, _default); }
float *tek_flag_ctx_float(TekFlagContext *ctx, const char *name, const char *help, float _default)
{
  TekFlag *f = tek_new_flag(ctx, Float, name, help);
  f->ref = &f->value.as_float;
  *(float*)f->ref = _default;
  f->_default.as_float = _default;
  return f->ref;
}
void tek_flag_float_var(const char *name, const char *help, float *variable, float _default) { tek_flag_ctx_float_var(&global_flag_context, name, help, variable, _default); }
void tek_flag_ctx_float_var(TekFlagContext *ctx, const char *name, const char *help, float *variable, float _default)
{
  TekFlag *f = tek_new_flag(ctx, Float, name, help);
  f->ref = variable;
  *(float*)f->ref = _default;
  f->_default.as_float = _default;
}

double *tek_flag_double(const char *name, const char *help, double _default) { return tek_flag_ctx_double(&global_flag_context, name, help, _default); }
double *tek_flag_ctx_double(TekFlagContext *ctx, const char *name, const char *help, double _default)
{
  TekFlag *f = tek_new_flag(ctx, Double, name, help);
  f->ref = &f->value.as_double;
  *(double*)f->ref = _default;
  f->_default.as_double = _default;
  return f->ref;
}
void tek_flag_double_var(const char *name, const char *help, double *variable, double _default) { tek_flag_ctx_double_var(&global_flag_context, name, help, variable, _default); }
void tek_flag_ctx_double_var(TekFlagContext *ctx, const char *name, const char *help, double *variable, double _default)
{
  TekFlag *f = tek_new_flag(ctx, Double, name, help);
  f->ref = variable;
  *(double*)f->ref = _default;
  f->_default.as_double = _default;
}

char **tek_flag_cstr(const char *name, const char *help, const char *_default) { return tek_flag_ctx_cstr(&global_flag_context, name, help, _default); }
char **tek_flag_ctx_cstr(TekFlagContext *ctx, const char *name, const char *help, const char *_default)
{
  TekFlag *f = tek_new_flag(ctx, Cstr, name, help);
  f->ref = &f->value.as_cstr;
  *(char**)f->ref = (char*)_default;
  f->_default.as_cstr = (char*)_default;
  return f->ref;
}
void tek_flag_cstr_var(const char *name, const char *help, char **variable, const char *_default) { tek_flag_ctx_cstr_var(&global_flag_context, name, help, variable, _default); }
void tek_flag_ctx_cstr_var(TekFlagContext *ctx, const char *name, const char *help, char **variable, const char *_default)
{
  TekFlag *f = tek_new_flag(ctx, Cstr, name, help);
  f->ref = variable;
  *(char**)f->ref = (char*)_default;
  f->_default.as_cstr = (char*)_default;
}

bool tek_flag_parse(int argc, char *argv[]) { return tek_flag_parse_ctx(&global_flag_context, argc, argv); }
bool tek_flag_parse_ctx(TekFlagContext *ctx, int argc, char *argv[])
{
  argc--; argv++;

  while (argc > 0) {
    char *flag = argv[0];
    if (strncmp(flag, "--", 2) != 0) {
      argc--; argv++;
      continue;
    } else {
      flag += 2; // Skip dashes
    }

    char *equals = strchr(flag, '=');
    if (equals != NULL) {
      *equals = '\0';
      equals++;
    }

    for (int i = 0; i < ctx->length; i++) {
      TekFlag *f = ctx->items + i;
      if (strcmp(flag, f->name) != 0) {
        continue;
      }

      // TODO: Handle errors when parsing
      switch (f->kind) {
        case Bool:
          if (equals != NULL) {
            if (
              strcmp(equals, "true") == 0 ||
              strcmp(equals, "t") == 0 ||
              strcmp(equals, "on") == 0
            ) *(bool*)f->ref = true;

            if (
              strcmp(equals, "false") == 0 ||
              strcmp(equals, "f") == 0 ||
              strcmp(equals, "off") == 0
            ) *(bool*)f->ref = false;
          } else {
            *(bool*)f->ref = true;
          }
        break;
        case Int:
          if (equals == NULL) {
            argc--; argv++;
            equals = argv[0];
          }
          *(uint64_t*)f->ref = strtoull(equals, NULL, 10);
        break;
        case Float:
          if (equals == NULL) {
            argc--; argv++;
            equals = argv[0];
          }
          *(float*)f->ref = strtof(equals, NULL);
        break;
        case Double:
          if (equals == NULL) {
            argc--; argv++;
            equals = argv[0];
          }
          *(double*)f->ref = strtod(equals, NULL);
        break;
        case Cstr:
          if (equals == NULL) {
            argc--; argv++;
            equals = argv[0];
          }
          *(char**)f->ref = equals;
        break;
      }

      // Next flag
      argc--; argv++;
    }
  }

  return true;
}

void tek_flag_print() { tek_flag_print_ctx(&global_flag_context); }
void tek_flag_print_ctx(TekFlagContext *ctx)
{
  // TODO: Implement
}

#endif // !TEK_IMPLEMENTATION

#endif // !TEK_H_
