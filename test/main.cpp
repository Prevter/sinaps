#include <iostream>
#include <sinaps.hpp>

static constexpr uint8_t TEST_STRING[] = "Hello, World! This is a test string to check if the pattern matching works.";

int main() {
    auto res = sinaps::find<sinaps::mask::string<"test string">>(TEST_STRING, sizeof(TEST_STRING));
    std::cout << "Found at index: " << res << std::endl;
    return 0;
}