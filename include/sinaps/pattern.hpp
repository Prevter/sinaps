#pragma once
#ifndef SINAPS_PATTERN_HPP
#define SINAPS_PATTERN_HPP

#include <array>
#include <tuple>
#include <cstdint>
#include <cstddef>

#include "token.hpp"
#include "utils.hpp"

namespace sinaps {
    /// @brief Primary container for mask patterns. It allows for easy pattern creation at compile-time.
    /// @tparam Mask List of masks that make up the pattern (e.g. mask::string<"abc">, mask::any<3>, etc.)
    template <typename... Mask>
    struct pattern {
        constexpr pattern() = default;
        constexpr pattern(std::tuple<Mask...>) {} // to allow for aggregate initialization

        // contains all the tokens (even zero-sized ones)
        static constexpr std::tuple raw_value = std::tuple_cat(Mask::value...);
        // size of the raw_value tuple
        static constexpr size_t raw_size = std::tuple_size_v<decltype(raw_value)>;
        // size of the pattern bytes
        static constexpr size_t size = (Mask::size + ...);

        // whether the pattern contains a cursor token
        static constexpr bool has_cursor = []<size_t... I>(std::index_sequence<I...>) {
            return ((std::get<I>(raw_value).type == token_t::type_t::cursor) || ...);
        }(std::make_index_sequence<raw_size>());

        // position of the cursor token in the pattern
        static constexpr size_t cursor_pos = []<size_t... I>(std::index_sequence<I...>) {
            size_t pos = 0;
            ((std::get<I>(raw_value).type == token_t::type_t::cursor ? (void) (pos = I) : void()), ...);
            return pos;
        }(std::make_index_sequence<raw_size>());

        // array of raw types (extracted from the raw_value tuple)
        static constexpr auto raw_types = []<size_t... I>(std::index_sequence<I...>) {
            return std::array<token_t::type_t, raw_size>{std::get<I>(raw_value).type...};
        }(std::make_index_sequence<raw_size>());

        // array of raw bytes (extracted from the raw_value tuple)
        static constexpr auto raw_bytes = []<size_t... I>(std::index_sequence<I...>) {
            return std::array<uint8_t, raw_size>{std::get<I>(raw_value).byte...};
        }(std::make_index_sequence<raw_size>());

        // array of tokens (excluding zero-sized ones)
        static constexpr auto value = []<size_t... I>(std::index_sequence<I...>) {
            std::array<token_t, size> value;
            size_t index = 0;
            ((std::get<I>(raw_value).type != token_t::type_t::cursor
                  ? (void) (value[index++] = std::get<I>(raw_value))
                  : void()), ...);
            return value;
        }(std::make_index_sequence<raw_size>());

        #define EXTRACT_VALUES(type, val) \
            []<size_t... I>(std::index_sequence<I...>) { \
                return std::array<type, sizeof...(I)>{ std::get<I>(value).val... }; \
            }(std::make_index_sequence<size>())

        // array of token types (excluding zero-sized ones)
        static constexpr auto types = EXTRACT_VALUES(token_t::type_t, type);
        // array of token bytes (excluding zero-sized ones)
        static constexpr auto bytes = EXTRACT_VALUES(uint8_t, byte);

        #undef EXTRACT_VALUES

        static consteval size_t count_string_length() {
            size_t length = 0;
            for (size_t i = 0; i < raw_size; i++) {
                if (raw_types[i] == token_t::type_t::byte) {
                    length += 3;
                } else {
                    length += 2;
                }
            }
            return length - 1;
        }

        template <size_t N = count_string_length()>
        static constexpr utils::FixedString<N + 1> to_string() {
            utils::FixedString<N + 1> str;
            size_t index = 0;
            for (size_t i = 0; i < raw_size; i++) {
                auto type = raw_types[i];
                if (type == token_t::type_t::byte) {
                    auto hex = hex_to_string(raw_bytes[i]);
                    str[index++] = hex[0];
                    str[index++] = hex[1];
                } else if (type == token_t::type_t::wildcard) {
                    str[index++] = '?';
                } else {
                    str[index++] = '^';
                }
                str[index++] = ' ';
            }
            str[index - 1] = '\0';
            return str;
        }
    };

    namespace impl {
        template <utils::FixedString S>
        constexpr size_t sizeOfPatternString() {
            size_t size = 0;
            for (size_t i = 0; i < S.size(); i++) {
                if (S[i] == ' ') {
                    continue;
                }
                size++;
                if (S[i] != '?' && S[i] != '^') {
                    i++;
                }
            }
            return size;
        }

        template <utils::FixedString S, size_t N = sizeOfPatternString<S>()>
        constexpr std::array<token_t, N> tokenizePatternString() {
            std::array<token_t, N> tokens;

            size_t index = 0;
            for (size_t i = 0; i < S.size(); i++) {
                if (S[i] == ' ') {
                    continue;
                }
                if (S[i] == '?') {
                    tokens[index++] = token_t();
                } else if (S[i] == '^') {
                    tokens[index++] = token_t(token_t::type_t::cursor);
                } else {
                    uint8_t byte = utils::from_hex(S[i]) << 4 | utils::from_hex(S[i + 1]);
                    tokens[index++] = token_t(byte);
                    i++;
                }
            }

            return tokens;
        }

        template <token_t Token>
        consteval auto unwrapToken() {
            if constexpr (Token.type == token_t::type_t::cursor) {
                return std::tuple{mask::cursor{}};
            } else if constexpr (Token.type == token_t::type_t::byte) {
                return std::tuple{mask::byte<Token.byte>{}};
            } else {
                return std::tuple{mask::any{}};
            }
        }

        template <size_t N, std::array<token_t, N> Tokens, size_t I = 0>
        consteval auto unwrapTokens() {
            if constexpr (I == N) {
                return std::tuple{};
            } else {
                return std::tuple_cat(unwrapToken<Tokens[I]>(), unwrapTokens<N, Tokens, I + 1>());
            }
        }

        /// @brief Get a pattern type from a list of tokens.
        template <std::array P>
        using make_pattern = decltype(pattern{unwrapTokens<P.size(), P>()});
    }

    namespace mask {
        /// @brief Creates a pattern mask from a string literal.
        template <utils::FixedString S>
        using pattern = impl::make_pattern<impl::tokenizePatternString<S>()>;
    }
}

#endif // SINAPS_PATTERN_HPP
