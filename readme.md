# Simple Arithmetic Program
### Demo expressions:
```
Input: x = 3; y = 5; x > y; x <= y; sqrt(x = (y == 3)); sin(x) + y + cos(y);...
```

### Supported Features
1. Direct calculate from expressions involving numerics, common math functions and variables.
2. Support arithmetic expressions. Boolean expressions evaluate to 1 if true. Variables evaluate to *true* in a boolean expression if its value is not 0.
3. Support initialization and assignment of variables. One variable a time, or separated with semicolons.
4. Arbitrary precision supported for common operations.
5. Supports *history*. Press Up/Down to select a history to enter.
6. Reduced space complexity (most operations are done in place)
7. Implemented some efficient algorithms to speed up the calculation

### Arithmetic Implementations
#### Multiplication
*Supports arbitrary precision*
Karatsuba's Method
#### Division, Modulus and Sqrt(), raise, ln 
*Supports arbitrary precision*
Simple iterative division, Newton's Iteration, Lookup Table. Recursive divide-and-conquer algorithms.
#### Sin, Cos, Exp
*Supports limited precision*
LUT (Lookup Table)/Math.h

### Variables
Supports variable assignment (name can only start with letters, containing letters and digits and underscore '_')


### Difficulties
1. How to ensure the precision of the functions? Using newton's iteration method or taylor series the error must be estimated and correctly controlled.
2. How to efficiently do multiplication, division, sqrt() and other math functions?
3. How to efficiently optimize simple operations in ``number.c`` such that those non-primitive operations (operations except for *addition* and *subtraction*) could be faster?
4. How to efficiently manage memory such that we can better reuse the resource already allocated and avoid frequent malloc()/realloc()/free() calls (which are very expensive operations) ?
5. How to parse the expression efficiently? Infix expression -> Postfix expression -> Stack Evaluation. Tokenization is performed on a custom structure, and we need to efficiently parse a token tree.
6. How to implement the lookup table (LUT) for variables ? -> Hash Table.
7. How to write the library so that they can later on be reused without significant modification? Is it serious enough?
8. Keep the comments and documents in detail so that people *won't be confused* when examining the source code.
9. Dividing the program into multiple libraries and corresponding header files that contain *definitions for structures, constants and global variables* and *prototypes* for functions.
10. Hide the implementation detail from the caller to ensure that when API changes, no significant change needs to be made in the source library. Also, it would be easier to change the internal implementation afterwards ***without affecting the calling procedure***.
11. Possible concurrency: The original number won't be modified during operations (unless it is being processed by internal *refactor functions*). However, it is ***Undefined Bahaviour*** to modify an argument during processing.
12. Multiple include files: possible nested duplicated includes. Solution: ``#ifndef`` and ``#define_HEADER_H``.
13. How to handle exceptions during process if there is any? My approach: function pointers that process such exceptions.
14. Portability. Using ``<ctype.h>`` and other library routines to ensure portability of the program. (For example, application on a non ASCII coding for characters, though very rarely to see).