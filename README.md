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
* Babble's current data types include 64bit unsigned integers and strings. Babble is statically typed, and all data besides integers must be declared with an explicit type (subject to change), using the `expr` construct. `expr (<type>, <expression>)` represents an expression of type `type`. Beyond initializing data, `expr` can currently be used to perform basic logical operations on integers (using the `bool` type), and will support more comprehensive expressions in the future. Operations on integers are currently limited to addition, and will be shortly expanded. See the examples for more information.

Control structures:
* Babble supports loops using the `wrep` and `rep` constructs, and branching with `if`. `wrep` is a while loop; `wrep (x)`, where `x` is an integer, will run the following block as long as `x` is non-zero. `if` functions similarly. `rep` is a static for loop, where `rep (x)` runs exactly `x` times, with no conditional evaluation. 
* Scoping is supported

I/O:
* Babble does not currently provide any mechanism for input. Output is exclusively to stdout, using `print`, and is available for all data types.

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
 
