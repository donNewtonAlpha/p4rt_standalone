#ifndef P4RT_STANDALONE_MACROS_H_
#define P4RT_STANDALONE_MACROS_H_

// Stringify the result of expansion of a macro to a string
// e.g:
// #define A text
// STRINGIFY(A) => "text"
// Ref: https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
#define STRINGIFY_INNER(s) #s
#define STRINGIFY(s) STRINGIFY_INNER(s)

#endif