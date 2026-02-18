# Classes Style

1. We avoid unneccessary function declartion that are do not provide required functionality.
2. We adhere to modern C++ standard where possible opting for intelligent usage of the C++ 20 standard.
3. Please utilise type hits where neccessary like `[[nodiscard]]`.
4. Class members should be prepended with `m_someVar`, this indicates a class member.
5. We prefer references over pointers for function arguments, as they're safer and avoid unnecessary `null_ptr` checks.

