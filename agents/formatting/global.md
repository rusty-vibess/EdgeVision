# Global Style

* All code should adhere to the rules laid out in `.clang-format`. 

Ensure to utilise `clang-format`, it should be available to you to call on files.

* We follow CamelCase for a majority of the code base unless in a snake_case typical place.

* Utilise smart pointers and references where possible.

* We prefer the `.hpp` / `.cpp` split across the repo for concrete code, even when the implementation is small.

* Header-only files should be reserved for simple transport or value types with little to no business logic.

* Repo structure conventions should be followed consistently:
  * Types belong under `include/types`.
  * Abstract interfaces belong under `include/interfaces`.
  * Abstract interfaces should also have a matching `.cpp` in `src/interfaces` for out-of-line definitions such as destructors.

* Test source file names should match the unit under test directly. Avoid placeholder suffixes such as `Header` in test filenames.

* If statements should be always followed by a new line
Example:
```cpp
if (condition) {
    // Coditional block
}

// Code continues
```

Unless it is a contiguous `if`, `elif`, `else` block, then the new line follows the control flow block.
Example:
```cpp
if (condition) {
    // Coditional block
} else {
    // Coditional block
}

// Code continues
```
