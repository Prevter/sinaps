#pragma once
#ifndef SINAPS_TOKEN_HPP
#define SINAPS_TOKEN_HPP

namespace sinaps {
    struct token_t {
        enum class type_t : uint8_t {
            byte,
            wildcard,
            cursor
        } type;

        union {
            uint8_t byte;
        };

        constexpr token_t(uint8_t byte) : type(type_t::byte), byte(byte) {}
        constexpr token_t(type_t type) : type(type), byte(0) {}
        constexpr token_t() : type(type_t::wildcard), byte(0) {}
    };
}

#endif // SINAPS_TOKEN_HPP
