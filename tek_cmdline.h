/*
  tek_cmdline - A utility library for handling commands/subcommands and flags.

  Command/Subcommand management:
  ------------------------------

    This library allows you to specify a command and it's subcommands and then have it automatically parse the commandline
    arguements to have it call the corresponding function. It will greedily pull commands from the arguement list until
    if finds something that isn't a known command name, parse the flags for that command (if it has any), and then pass
    the remaining arguements into the registered command function.

    command subcommand [flags] // Works
    command [flags] subcommand // Does not work

    It is the responsibility of the owner to free any memory allocated with the command (if it isn't stack allocated).

    Example:
    ```
      int sub(TekcCmd *cmd, int argc, char *argv[])
      {
        bool test;
        if (!tekc_getflag_bool(&cmd->flags, "test", &test)) {
          printf("Failed to get test flag\n");
          exit(1);
        }

        // Do something
      }

      int main(int argc, char *argv[])
      {
        TekcCmd rootcmd = {
            .name = "command",
            .usage = "Some usage string that will print out when command help is printed.",
        };

        TekcCmd subcmd = {
            .name = "subcommand",
            .brief = "Something that will be printed for this subcommand when parent command help is printed.",
            .usage = "Some usage string that will print out when command help is printed.",
            .run = sub,
        };
        tekc_flag_bool(&subcmd.flags, "t", "test", "A test flag", false);
        tekc_cmd_add_subcmd(&rootcmd, &subcmd);

        tekc_cmd_run(&rootcmd, argc, argv);
        tekc_cmd_free(&rootcmd); // Recursively free all flags from all registered commands/subcommands
      }
    ```

  Flag management:
  ----------------

    This library allows for you to register commandline flags and parse them. It allows for
    flags of the following types:
      * bool
      * uint64_t
      * float
      * double
      * char*
    The parse function and registrations functions require that a long name and default value is given, and allow
    for optional short names and description.

    FlagContexts MUST be freed or their memory will leak.

    Boolean flags are allowed in this format:
      --bool || --bool=true || -b || -b=true

    All other flags are allowed in this format:
      --other=value || --other value || -o=value || -o value

    Example:
    ```
      TekcFlagContext ctx = {0}

      bool test1;
      tekc_flag_bool_var(&ctx, "t", "test", "A test flag", false);

      bool *test2 = tekc_flag_bool(&ctx, "x", "xtest", "A second test flag", false);

      if (!tekc_flag_parse(&ctx, argc, argv, NULL)) {
        printf("Failed to parse flags\n");
        exit(1);
      }

      tekc_flag_free(&ctx); // Flags must be freed
    ```
*/

#ifndef TEK_CMDLINE_H_
#define TEK_CMDLINE_H_

#include <stdbool.h>
#include <stdint.h>

#ifndef TEK_CMDLINE_ASSERT
#include <assert.h>
#define TEK_CMDLINE_ASSERT assert
#endif

#if defined(TEK_CMDLINE_MALLOC) && defined(TEK_CMDLINE_FREE) && defined(TEK_CMDLINE_REALLOC)
#elif !defined(TEK_CMDLINE_MALLOC) && !defined(TEK_CMDLINE_FREE) && !defined(TEK_CMDLINE_REALLOC)
#else
#error "Must define all or none of TEK_CMDLINE_MALLOC, TEK_CMDLINE_FREE, and TEK_CMDLINE_REALLOC."
#endif
#ifndef TEK_CMDLINE_MALLOC
#include <stdlib.h>
#define TEK_CMDLINE_MALLOC malloc
#define TEK_CMDLINE_REALLOC realloc
#define TEK_CMDLINE_FREE free
#endif

// Slices can be any struct that has a pointer to items, length (size_t), and
// capacity (size_t)
#define TEK_CMDLINE_SLICE_INIT_CAPACITY 8

#define tekc_slice_reserve(slice, expected)                                                                                                \
  do {                                                                                                                                     \
    if ((slice)->capacity == 0) (slice)->capacity = TEK_CMDLINE_SLICE_INIT_CAPACITY;                                                       \
    while ((expected) > (slice)->capacity) (slice)->capacity *= 2;                                                                         \
    (slice)->items = TEK_CMDLINE_REALLOC((slice)->items, sizeof(*(slice)->items) * (slice)->capacity);                                     \
    TEK_CMDLINE_ASSERT((slice)->items != NULL && "Realloc failed");                                                                        \
  } while (0)

#define tekc_slice_append(slice, item)                                                                                                     \
  do {                                                                                                                                     \
    tekc_slice_reserve((slice), (slice)->length + 1);                                                                                      \
    (slice)->items[(slice)->length++] = (item);                                                                                            \
  } while (0)

#define tekc_slice_remove_unordered(slice, index)                                                                                          \
  do {                                                                                                                                     \
    size_t i = (index);                                                                                                                    \
    TEK_CMDLINE_ASSERT(i < (slice)->length && "Removal out of bounds");                                                                    \
    (slice)->items[i] = (slice)->items[--(slice)->length];                                                                                 \
  } while (0)

#define tekc_slice_free(slice)                                                                                                             \
  do {                                                                                                                                     \
    if ((slice)->items) TEK_CMDLINE_FREE((slice)->items);                                                                                  \
    (slice)->items = NULL;                                                                                                                 \
    (slice)->length = 0;                                                                                                                   \
    (slice)->capacity = 0;                                                                                                                 \
  } while (0)

typedef struct TekcFlag TekcFlag;
typedef struct TekcFlagContext TekcFlagContext;
typedef struct TekcArgs TekcArgs;
typedef struct TekcCmd TekcCmd;
typedef int (*TekcRunCmd)(TekcCmd *cmd, int argc, char *argv[]);

typedef enum {
  Bool = 0,
  Int,
  Float,
  Double,
  Cstr,
} TekcFlagKind;

typedef union {
  bool as_bool;
  uint64_t as_int;
  float as_float;
  double as_double;
  const char *as_cstr;
} TekcFlagValue;

struct TekcFlag {
  TekcFlagKind kind;
  TekcFlagValue value;
  TekcFlagValue _default;
  void *reference;
  const char *sname;
  const char *lname;
  const char *help;
};

struct TekcFlagContext {
  TekcFlag *items;
  size_t length, capacity;

  char *error;
  int error_index;
};

struct TekcArgs {
  char **items;
  size_t length, capacity;
};

struct TekcCmd {
  char *name;
  char *brief;
  char *usage;
  TekcRunCmd run;
  TekcFlagContext flags;

  TekcCmd *_subcommand;
  TekcCmd *_next;
};

#define TEK_CMDLINE_FLAG_TYPEDEFS                                                                                                          \
  X(bool, bool, Bool)                                                                                                                      \
  X(uint64_t, int, Int)                                                                                                                    \
  X(float, float, Float)                                                                                                                   \
  X(double, double, Double)                                                                                                                \
  X(const char *, cstr, Cstr)

void tekc_cmd_add_subcmd(TekcCmd *cmd, TekcCmd *subcmd);
int tekc_cmd_run(TekcCmd *cmd, int argc, char *argv[]);
void tekc_cmd_help(TekcCmd *cmd);
void tekc_cmd_free(TekcCmd *cmd);
bool tekc_flag_parse(TekcFlagContext *ctx, int argc, char *argv[], TekcArgs *remaining);
void tekc_flag_error(TekcFlagContext *ctx, char **error, TekcFlag **flag);
void tekc_flag_print(TekcFlagContext *ctx);
void tekc_flag_free(TekcFlagContext *ctx);

#define X(TYPE, NAME, ENUM)                                                                                                                \
  TYPE *tekc_flag_##NAME(TekcFlagContext *ctx, const char *sname, const char *lname, const char *help, TYPE _default);                     \
  int tekc_flag_##NAME##_var(TekcFlagContext *ctx, const char *sname, const char *lname, const char *help, TYPE *variable, TYPE _default); \
  bool tekc_getflag_##NAME(TekcFlagContext *ctx, const char *lname, TYPE *reference);
TEK_CMDLINE_FLAG_TYPEDEFS
#undef X

#endif // !TEK_CMDLINE_H_

////////////////////////////////////////////////////////////////////////////

#ifdef TEK_CMDLINE_IMPLEMENTATION
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

void tekc_cmd_add_subcmd(TekcCmd *cmd, TekcCmd *subcmd)
{
  if (!cmd) return;

  if (!cmd->_subcommand) {
    cmd->_subcommand = subcmd;
  } else {
    subcmd->_next = cmd->_subcommand;
    cmd->_subcommand = subcmd;
  }
}

int tekc_cmd_run(TekcCmd *cmd, int argc, char *argv[])
{
  if (!cmd) return -1;
  TekcCmd *current = cmd;
  argc--, argv++; // Skip arg0

  bool found;
  do {
    found = false;
    for (TekcCmd *sub = current->_subcommand; sub; sub = sub->_next) {
      if (!strcmp(sub->name, argv[0])) {
        current = sub;
        argc--, argv++;
        found = true;
        break;
      }
    }
  } while (found);

  // If there is no run command, treat as a nop
  if (!current->run) return 0;

  // Parse flags for currently selected commands.
  if (current->flags.length) {
    TekcArgs remaining = {0};
    if (!tekc_flag_parse(&current->flags, argc, argv, &remaining)) {
      // FIXME: Give the caller a better error somehow. Return args? "error" and "error_ctx" fields on the Cmd struct?
      tekc_slice_free(&remaining);
      return -1;
    }
    int code = current->run(current, remaining.length, remaining.items);
    tekc_slice_free(&remaining);
    return code;
  }

  return current->run(current, argc, argv);
}

void tekc_cmd_help(TekcCmd *cmd)
{
  printf("%s\n\n", cmd->usage);

  if (cmd->_subcommand) printf("    Subcommands:\n");
  for (TekcCmd *sub = cmd->_subcommand; sub; sub = sub->_next) {
    printf("        %s\n            %s\n", sub->name, sub->brief);
  }

  tekc_flag_print(&cmd->flags);
}

void tekc_cmd_free(TekcCmd *cmd)
{
  if (!cmd) return;

  if (cmd->flags.length) tekc_flag_free(&cmd->flags);

  TekcCmd *sub = cmd->_subcommand;
  TekcCmd *next;
  while (sub) {
    next = sub->_next;
    tekc_cmd_free(sub);
    sub = next;
  }
}

static TekcFlag *tekc_flag_new_flag(TekcFlagContext *ctx, TekcFlagKind kind, const char *sname, const char *lname, const char *help)
{
  TekcFlag flag = {0};
  tekc_slice_append(ctx, flag);
  TekcFlag *f = &ctx->items[ctx->length - 1];
  f->kind = kind;
  f->sname = sname;
  f->lname = lname;
  f->help = help;
  return f;
}

bool tekc_flag_parse(TekcFlagContext *ctx, int argc, char *argv[], TekcArgs *remaining)
{
  while (argc > 0) {
    char *flag = argv[0];
    bool lname = !strncmp(flag, "--", 2);
    bool sname = !lname && (flag[0] == '-');

    if (!lname && !sname) {
      if (remaining) tekc_slice_append(remaining, argv[0]);
      argc--, argv++;
      continue;
    } else if (lname) {
      flag += 2;
    } else {
      flag++;
    }

    char *equals = strchr(flag, '=');
    if (equals) equals++;

    bool found = false;
    for (int i = 0; i < ctx->length; i++) {
      TekcFlag *f = ctx->items + i;
      if (lname && strcmp(flag, f->lname)) continue;
      if (sname && strcmp(flag, f->sname)) continue;
      found = true;

      char *end;
      switch (f->kind) {
      case Bool:
        if (equals) {
          if (!strcmp(equals, "true") || !strcmp(equals, "t") || !strcmp(equals, "on")) *(bool *)f->reference = true;

          if (!strcmp(equals, "false") || !strcmp(equals, "f") || !strcmp(equals, "off")) *(bool *)f->reference = false;
        } else {
          *(bool *)f->reference = true;
        }
        break;
      case Int:
        if (!equals) {
          argc--, argv++;
          equals = argv[0];
        }
        if (!equals) {
          ctx->error = "Missing value in integer flag.";
          ctx->error_index = i;
          return false;
        }

        *(uint64_t *)f->reference = strtoull(equals, &end, 10);
        if (errno != 0 || *end != '\0') {
          ctx->error = "Failed to parse value in integer flag.";
          ctx->error_index = i;
          return false;
        }
        break;
      case Float:
        if (!equals) {
          argc--, argv++;
          equals = argv[0];
        }
        if (!equals) {
          ctx->error = "Missing value in float flag.";
          ctx->error_index = i;
          return false;
        }

        *(float *)f->reference = strtof(equals, &end);
        if (errno != 0 || *end != '\0') {
          ctx->error = "Failed to parse value in float flag.";
          ctx->error_index = i;
          return false;
        }
        break;
      case Double:
        if (!equals) {
          argc--, argv++;
          equals = argv[0];
        }
        if (!equals) {
          ctx->error = "Missing value in double flag.";
          ctx->error_index = i;
          return false;
        }

        *(double *)f->reference = strtod(equals, &end);
        if (errno != 0 || *end != '\0') {
          ctx->error = "Failed to parse value in double flag.";
          ctx->error_index = i;
          return false;
        }
        break;
      case Cstr:
        if (!equals) {
          argc--, argv++;
          equals = argv[0];
        }
        if (!equals) {
          ctx->error = "Missing value in cstr flag.";
          ctx->error_index = i;
          return false;
        }

        *(char **)f->reference = equals;
        break;
      }

      if (!found) {
        // FIXME: Give the caller a better error without allocating.
        ctx->error = "Invalid flag";
        return false;
      }

      // Next flag
      argc--, argv++;
    }
  }

  return true;
}

void tekc_flag_error(TekcFlagContext *ctx, char **error, TekcFlag **flag)
{
  if (error) *error = ctx->error;
  if (flag) {
    *flag = ctx->error_index < 0 ? NULL : &ctx->items[ctx->error_index];
  }
}

void tekc_flag_print(TekcFlagContext *ctx)
{
  for (int i = 0; i < ctx->length; i++) {
    TekcFlag *flag = ctx->items + i;
    printf("\t");
    if (flag->sname) printf("-%s, ", flag->sname);
    printf("--%s ", flag->lname);
    switch (flag->kind) {
    case Bool:
      printf("(default: %s)\n", flag->_default.as_bool ? "true" : "false");
      break;
    case Int:
      printf("(default: %" PRIu64 ")\n", flag->_default.as_int);
      break;
    case Float:
      printf("(default: %f)\n", flag->_default.as_float);
      break;
    case Double:
      printf("(default: %f)\n", flag->_default.as_double);
      break;
    case Cstr:
      if (flag->_default.as_cstr) {
        printf("(default: \"%s\")\n", flag->_default.as_cstr);
      } else {
        printf("(default: null)\n");
      }
      break;
    }
    if (flag->help) {
      printf("\t\t%s\n\n", flag->help);
    } else {
      printf("\n");
    }
  }
}

void tekc_flag_free(TekcFlagContext *ctx)
{
  tekc_slice_free(ctx);
}

#define X(TYPE, NAME, ENUM)                                                                                                                \
  TYPE *tekc_flag_##NAME(TekcFlagContext *ctx, const char *sname, const char *lname, const char *help, TYPE _default)                      \
  {                                                                                                                                        \
    if (!lname) return NULL;                                                                                                               \
    TekcFlag *f = tekc_flag_new_flag(ctx, ENUM, sname, lname, help);                                                                       \
    f->reference = &f->value.as_##NAME;                                                                                                    \
    *(TYPE *)f->reference = _default;                                                                                                      \
    f->_default.as_##NAME = _default;                                                                                                      \
    return f->reference;                                                                                                                   \
  }                                                                                                                                        \
                                                                                                                                           \
  int tekc_flag_##NAME##_var(TekcFlagContext *ctx, const char *sname, const char *lname, const char *help, TYPE *variable, TYPE _default)  \
  {                                                                                                                                        \
    if (!lname) return -1;                                                                                                                 \
    TekcFlag *f = tekc_flag_new_flag(ctx, ENUM, sname, lname, help);                                                                       \
    f->reference = variable;                                                                                                               \
    *(TYPE *)f->reference = _default;                                                                                                      \
    f->_default.as_##NAME = _default;                                                                                                      \
    return 0;                                                                                                                              \
  }                                                                                                                                        \
                                                                                                                                           \
  bool tekc_getflag_##NAME(TekcFlagContext *ctx, const char *lname, TYPE *reference)                                                       \
  {                                                                                                                                        \
    for (int i = 0; i < ctx->length; i++) {                                                                                                \
      if (strcmp(lname, ctx->items[i].lname)) continue;                                                                                    \
      if (ctx->items[i].kind != ENUM) return -1;                                                                                           \
      *reference = *(TYPE *)(ctx->items[i].reference);                                                                                     \
      return true;                                                                                                                         \
    }                                                                                                                                      \
    return false;                                                                                                                          \
  }
TEK_CMDLINE_FLAG_TYPEDEFS
#undef X

#endif // !TEK_CMDLINE_IMPLEMENTATION
