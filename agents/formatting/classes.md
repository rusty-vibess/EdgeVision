# Classes Style

1. Avoid unnecessary function declarations that do not provide required functionality.
2. The repo targets modern C++20. Use the language and standard library deliberately rather than falling back to older C-style patterns.
3. Please utilise type hints where necessary such as `[[nodiscard]]`.
4. Class members should be prepended with `m_someVar`, this indicates a class member.
5. We prefer references over pointers for function arguments, as they're safer and avoid unnecessary `nullptr` checks.
6. Abstract interfaces should use the repo interface layout conventions in either `include/interfaces` or module-prefixed `include/<module>/interfaces`.
7. Concrete public class headers should live under their module include path, for example `include/viewer`.
