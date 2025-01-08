## SINAPS

Simple signature scanner.

> **Note**: This project is still in development and doesn't have all planned features yet.

### Features
- **Compile-time pattern generation**: Patterns are generated at compile-time, so there is no runtime overhead.
- **Custom index cursor**: You can set the index cursor to any position in the pattern,
allowing you to find a specific occurrence of the pattern. 
- **Pattern builder**: You can build patterns using a simple and intuitive syntax.

### Usage
```cpp
#include <sinaps.hpp>

static constexpr uint8_t STRING[] = "Test buffer 123 123 123.";

int main() {
    auto res = sinaps::find<
        sinaps::mask::sequence<3,
            sinaps::mask::string<"123">,
            sinaps::mask::any
        >
    >(STRING, sizeof(STRING));

    if (res == -1) {
        std::cout << "Pattern not found!" << std::endl;
    } else {
        std::cout << "Pattern found at index " << res << std::endl; // 12
    }
    
    return 0;
}
```