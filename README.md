Babble v1.4
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
* Babble's data types include 64bit integers and strings.

Control structures:
* ...

Misc:
* ...

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
**v1.4** (continued updates):
* Add all traditional arithmetic/bitwise operators

**v1.6**:
* Add functions
* Add global symbols, multiple object file support
 
