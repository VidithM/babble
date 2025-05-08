Babble v1.2
## About:
Babble is a simple imperative programming language I'm working on to practice making a compiler. This code is a compiler implementation for Linux x86-64 using Netwide assembly.
### Examples/Usage:
To build, simply run `make` or `make debug` for debug mode (turns on asserts and debug printouts). You can also install in your PATH with `make install`. 
Babble supports a few gcc-like flags:
* `-g`: Compile with debug symbols, using the DWARF format
* `-o <name>`: Specifies a name for the executable
e.g. `babble -g -o temp temp.bbl`

**Language details**:

Data types:
* Babble currently only has 64-bit integers as the sole data type

Control structures:
* Only loops are available right now, used with the `rep` keyword. This is similar to a for-loop in other languages, except there is no condition evaluated each iteration. The loop is simply run for the specified number of iterations (e.g. `rep (4)` runs 4 times).

Misc:
* Scopes are supported

**Example code**:

```
% This is a comment
x = 4
print (x);

iter = 0;
rep (x) {
    % This loop will run 4 times. There is no conditional evaluation for each iteration.
    print (iter);
    iter += 1;
}
```
* See more examples in [examples/](https://github.com/vidithm/babble/tree/v1.4/examples)

### Future plans:
**v1.4**:
* Support string literals
* Support conditionals/branching
 
