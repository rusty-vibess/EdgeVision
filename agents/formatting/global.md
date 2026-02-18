# Global Style

* All code should adhere to the rules laid out in `.clang-format`. 

Ensure to utilise `clang-format`, it should be available to you to call on files.

* We follow CamelCase for a majority of the code base unless in a snake_case typical place.

* Utilise smart pointers and references where possible.

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
