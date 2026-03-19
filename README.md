# tek.h

A lightweight, header-only C library providing utility macros for dynamic arrays (slices) and a robust command-line flag parsing system.

## Features

* **Header-Only:** Easy to drop into any C project.
* **Slices (Dynamic Arrays):** "Type-safe" dynamic array macros that work with any custom struct containing `items`, `length`, and `capacity`.
* **Flag Parsing:** Full-featured command-line argument parsing with support for booleans, integers, floats, doubles, and strings.
* **Contexts:** Global and isolated context options for flag parsing.
* **Auto-Generated Help:** Built-in capability to format and print flag descriptions and defaults.

---

## Getting Started

Because `tek.h` is a header-only library, you must define the implementation in exactly **one** C file before including it.

```c
// main.c
#define TEK_IMPLEMENTATION
#include "tek.h"

int main(int argc, char **argv) {
    // Your code here
    return 0;
}
