# C coding style 

Mostly from https://www.kernel.org/doc/html/latest/process/coding-style.html with some overrides at the top of this document

This is a short document describing the preferred coding style for the m_jmmer . Coding style is very personal, and I won’t force my views on anybody, but this is what goes for anything that I have to be able to maintain, and I’d prefer it for most other things too. Please at least consider the points made here.

First off, I’d suggest printing out a copy of the GNU coding standards, and NOT read it. Burn them, it’s a great symbolic gesture.

Anyway, here goes:

1) Indentation
Tabs are 8 characters, and thus indentations are also 8 characters. There are heretic movements that try to make indentations 4 (or even 2!) characters deep, and that is akin to trying to define the value of PI to be 3.

Rationale: The whole idea behind indentation is to clearly define where a block of control starts and ends. Especially when you’ve been looking at your screen for 20 straight hours, you’ll find it a lot easier to see how the indentation works if you have large indentations.

In short, 8-char indents make things easier to read, and have the added benefit of warning you when you’re nesting your functions too deep. Heed that warning.

## Struct

Structs use PascalCase; functions use snake_case. This keeps roles visually distinct.

**Bad**
```c
struct my_new_thingy{
        inx x; 
}my_new_thingy;
```

**Good**
```c
struct MyNewThingy {
        int x;
} MyNewThingy;
```

## Enums

Enums use SCREAMING_SNAKE_CASE and are typedef’d in this project (intentional override of kernel guidance).

**Bad**
```c
enum { // BAD missing the typedef
    Ok = 0,  // BAD NOT SCREAMING_SNAKE_CASE. BAD NOT 8 for Indentation
    Not_Ok,  // BAD NOT SCREAMING_SNAKE_CASE BAD NOT 8 for Indentation
}my_new_enm; // BAD NOT SCREAMING_SNAKE_CASE BAD NOT 8 for Indentation
```

**Good**
```c
typedef enum { // GOOD typedef is there
        OK = 0,    // GOOD SCREAMING_SNAKE_CASE and 8 for Indentation
        NOT_OK,    // GOOD SCREAMING_SNAKE_CASE and 8 for Indentation
}MY_NEW_ENUM;  // GOOD  SCREAMING_SNAKE_CASER 
```

## MACROS

Spacing rules in macro bodies matter because some preprocessors (especially in GCC, MSVC, and Clang) can misinterpret tightly packed tokens—like )( or __—as token-pasting, built-ins, or syntax errors.
Extra spaces make token boundaries explicit, preventing accidental merges and avoiding silent failures or unexpected behavior when the macro expands.

* **Macro Define** the first line that declares the macro name and parameters.
* **Macro Body**  the code after the \ continuation lines; this is where spacing rules matter to avoid silent preprocessor issues.

Good example (clear, compiler-agnostic spacing in the body):
```c
#define CHECK_EQ_I32(test_name, actual, expected) \
do { \
        int _ok_ = ( actual ) == ( expected ); \
        CHECK ( _ok_, "%s want %d got %d", \
            ( const char * ) ( test_name ), \
            ( int ) ( expected ), ( int ) ( actual ) ); \
} while ( 0 )
```

**Bad** 

Tight spacing, easier to misread, can break in some preprocessors:
```c
#define CHECK_EQ_I32(test_name,actual,expected)\
do{int _ok_=(actual)==(expected);CHECK(_ok_,"%s want %d got %d",(const char*)(test_name),(int)(expected),(int)(actual));}while(0)
```

## Comments. 

In my(personal) world comments are not meant to be in code. 

markdown files for API/module are a nice start. They like many other C projects are sorted in Documentation/MODULE_NAME 

This is where one can have "examples" and more in-depth documentation. 

Alot of the time one should be able explain a function in less than 80 char's

Single-line
IDC
```c
/* my notes for other developers */
// or this IDC 
``` 

Multi-line
```c
/*!
 * my notes for other developers
 * my notes for other developers X
*/
```

**Bad**

```c
/**
 * @brief Join two strings and print them to standard output.
 *
 * This function concatenates the string in @p val with the string in @p other_val
 * and prints the result to standard output. No additional delimiter or newline
 * is inserted between the two strings.
 *
 * @param val        First string to print. Must be a valid null-terminated string.
 * @param other_val  Second string to print immediately after @p val.
 *
 * @return Pointer to the first string (@p val).
 *
 * @note This function does not perform any memory allocation or modification
 *       of the input strings. It writes directly to `stdout` using printf.
 */
const char *join_and_print(const char *val, const char *other_val)
{
        printf("%s%s", val, other_val);
        return val;
}
```

**Good**
```c
// print two string values together to stdout. 
const char *join_and_print(const char *val, const char *other_val)
{
        printf("%s%s", val, other_val);
        return val;
}
```

**Bad**
```c
/**
 * @brief Represents a key-value pair with an optional default value.
 *
 * This structure holds a string key, its associated string value,
 * and an optional default value that can be used if the primary value is NULL.
 */
struct MyKeyValue {
        const char *key;           /**< The key string (non-owning pointer). */
        const char *value;         /**< The associated value string (non-owning pointer). */
        const char *default_value; /**< Default value used when @p value is NULL or empty.
                                        This avoids null checks in consuming code and provides
                                        a safe fallback. */
} MyKeyValue;
```


**Good**
```c
// String key, value w/default. 
struct MyKeyValue {
        const char *key;
        const char *value
        const char *default_value;
} MyKeyValue;

```

## Use of __ starting or ending with

Names starting with _  or __ are reserved for the compiler and standard library.
Using them in your functions or macros can cause conflicts or undefined behavior if the compiler already uses that name internally.
Sticking to non-reserved names ensures portability and avoids silent breakage across compilers and platforms.



## File naming and function naming
Filenames
* Use all lowercase with underscores.
* Pattern: <project>_<module>_<submodule>.<ext> (grouped under a directory for the module when helpful).
  * Example header/source pair: ```api/mcpkg_api_util.h api/mcpkg_api_util.c```
* Public functions (visible across translation units)
  * Prefix with project, module, and submodule:
    * mcpkg_api_util_do_something()
  * This makes call sites self‑describing and grep‑friendly in large code bases.
* Private (internal) functions inside a .c
  * Keep static helpers short but specific; no global‑style prefixes:
    * static int parse_flag(...), static void write_header(...)
  * Avoid generic names like process() or helper(). They clutter backtraces and confuse reviewers.
* Unit tests
  * Mirror the target unit’s name with a _tst suffix.
  * Example: tests for api/mcpkg_api_util.c live in tests/mcpkg_api_util_tst.c.
  * Test helpers shared across files go into tests/tst_macros.{h,c} (already in this repo).
* Headers
  * One module header per public API area; keep it focused.
  * Header guard: MCPKG_API_UTIL_H (ALLCAPS, path‑style).
  * Include order inside headers: the module’s own public header first (if split), project headers next, then system headers.
* Rationale
  * Names are longer, but you immediately know ownership and discoverability via grep is trivial.
  * Lowercase files play nice across filesystems (Linux/macOS/Windows).
  * Static helpers keep translation units tidy without leaking prefixes into the global namespace.

example: adding a file to the module api with the sub-module named util 

```api/mcpkg_api_util.h```

now all function in this submodule are declared as so

```mcpkg_api_util_do_something()```

I know that this can get troublesome when making large code bases as the function names get long. but when you look at it you know right away where it is coming from. 

## End of Kernel coding style guide line

.astyle file is in the base of this project to use it

```bash
astyle --options=.astyle --recursive "libmcpkg/*.c,*.h" "libmcpkg/*.h"
astyle --options=.astyle --recursive "tests/*.c,*.h" "tests/*.h"
astyle --options=.astyle --recursive "mcpkg/*.c,*.h" "mcpkg/*.h"
```

All other bits should folllow  https://www.kernel.org/doc/html/latest/process/coding-style.html
