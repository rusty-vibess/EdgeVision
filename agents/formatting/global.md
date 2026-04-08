# Global Style

* All code should adhere to the rules laid out in `.clang-format`.

Ensure to utilise `clang-format`, it should be available to you to call on files.

* We follow CamelCase for a majority of the code base unless in a snake_case typical place.

* The repo baseline is modern C++20.
  * Assume GCC 11.4.0 and `aarch64-linux-gnu`.
  * Prefer standard C++20 facilities when they improve clarity.

* We prefer the `.hpp` / `.cpp` split across the repo for concrete code, even when the implementation is small.

* Header-only files should be reserved for simple transport or value types with little to no business logic.

* Utilise smart pointers and references where possible.

* Prefer modern C++ conventions over C-style conventions.
  * Prefer C++ standard library headers and facilities over C stdio and C-style helper macros where practical.
  * Avoid `(void)`-style unused parameter suppression. Prefer unnamed parameters or `[[maybe_unused]]` where appropriate.
  * Avoid preprocessor-heavy compatibility shims unless there is a demonstrated portability need.
  * Prefer the canonical include path exported by a dependency instead of probing internal header layouts.

* Repo structure conventions should be followed consistently:
  * Module-owned public concrete headers belong under `include/<module>`.
  * These headers should be imported via the module path, for example `#include "reconstruction/ReconstructionPipeline.hpp"`.
  * Concrete implementation files should mirror that layout under `src/<module>` when they represent module-owned concrete classes.
  * Types belong under `include/types`.
  * Abstract interfaces belong under `include/interfaces`.
  * Abstract interfaces should also have a matching `.cpp` in `src/interfaces` for out-of-line definitions such as destructors.
  * Tests should mirror the code layout and live under matching subdirectories such as `tests/<module>`, `tests/interfaces`, and `tests/types`.

* Test source file names should match the unit under test directly. Avoid placeholder suffixes such as `Header` in test filenames.

* If statements should be always followed by a new line.
Example:
```cpp
if (condition) {
    // Conditional block
}

// Code continues
```

Unless it is a contiguous `if`, `else if`, `else` block, then the new line follows the control flow block.
Example:
```cpp
if (condition) {
    // Conditional block
} else {
    // Conditional block
}

// Code continues
```
