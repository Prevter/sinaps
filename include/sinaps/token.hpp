#pragma once
#ifndef SINAPS_TOKEN_HPP
#define SINAPS_TOKEN_HPP

#include <cstdint>

namespace sinaps {
    struct token_t {
        enum class type_t : uint8_t {
            byte,
            wildcard,
            cursor,
            masked
        } type;

        uint8_t byte;
        uint8_t mask = 0;

        constexpr token_t(uint8_t byte, uint8_t mask) : type(type_t::masked), byte(byte), mask(mask) {
            if (mask == 0) {
                type = type_t::wildcard;
            } else if (mask == 0xff) {
                type = type_t::byte;
            }
        }
        constexpr token_t(uint8_t byte) : type(type_t::byte), byte(byte) {}
        constexpr token_t(type_t type) : type(type), byte(0) {}
        constexpr token_t() : type(type_t::wildcard), byte(0) {}
    };
}

#endif // SINAPS_TOKEN_HPP
