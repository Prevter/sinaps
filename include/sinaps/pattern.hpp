#pragma once
#ifndef SINAPS_PATTERN_HPP
#define SINAPS_PATTERN_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>

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

        // array of raw masks (extracted from the raw_value tuple)
        static constexpr auto raw_masks = []<size_t... I>(std::index_sequence<I...>) {
            return std::array<uint8_t, raw_size>{std::get<I>(raw_value).mask...};
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
        // array of token masks (excluding zero-sized ones)
        static constexpr auto masks = EXTRACT_VALUES(uint8_t, mask);

        #undef EXTRACT_VALUES

        static consteval size_t count_string_length() {
            size_t length = 0;
            for (size_t i = 0; i < raw_size; i++) {
                if (raw_types[i] == token_t::type_t::byte) {
                    length += 3;
                } else if (raw_types[i] == token_t::type_t::masked) {
                    length += 6;
                } else {
                    length += 2;
                }
            }
            return length - 1;
        }

        template <size_t N = count_string_length()>
        static consteval utils::FixedString<N + 1> to_string() {
            utils::FixedString<N + 1> str;
            size_t index = 0;
            for (size_t i = 0; i < raw_size; i++) {
                auto type = raw_types[i];
                if (type == token_t::type_t::byte) {
                    auto hex = utils::hex_to_string(raw_bytes[i]);
                    str[index++] = hex[0];
                    str[index++] = hex[1];
                } else if (type == token_t::type_t::masked) {
                    auto hex = utils::hex_to_string(raw_bytes[i]);
                    str[index++] = hex[0];
                    str[index++] = hex[1];
                    str[index++] = '&';
                    hex = utils::hex_to_string(raw_masks[i]);
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
        consteval size_t sizeOfPatternString() {
            size_t size = 0;
            for (size_t i = 0; i < S.size(); i++) {
                if (S[i] == ' ') {
                    continue;
                }
                size++;
                if (utils::is_hex(S[i])) {
                    i++;
                }
                if (i + 1 < S.size() && S[i + 1] == '&') {
                    i += 3;
                }
                if (S[i] != '?' && S[i] != '^') {
                    i++;
                }
            }
            return size;
        }

        template <utils::FixedString S, size_t N = sizeOfPatternString<S>()>
        consteval std::array<token_t, N> tokenizePatternString() {
            std::array<token_t, N> tokens;

            size_t index = 0;
            for (size_t i = 0; i < S.size(); i++) {
                switch (S[i]) {
                    case ' ': continue;
                    case '?': tokens[index++] = token_t(); break;
                    case '^': tokens[index++] = token_t(token_t::type_t::cursor); break;
                    default: {
                        uint8_t byte = utils::from_hex(S[i]) << 4 | utils::from_hex(S[i + 1]);
                        tokens[index++] = token_t(byte);
                        i++;

                        // check for masked byte
                        if (i + 1 < S.size() && S[i + 1] == '&') {
                            uint8_t mask = utils::from_hex(S[i + 2]) << 4 | utils::from_hex(S[i + 3]);
                            tokens[index - 1] = token_t(byte, mask);
                            i += 3;
                        }
                    } break;
                }
            }

            return tokens;
        }

        template <token_t Token>
        consteval auto unwrapToken() {
            if constexpr (Token.type == token_t::type_t::cursor) {
                return mask::cursor{};
            } else if constexpr (Token.type == token_t::type_t::byte) {
                return mask::byte<Token.byte>{};
            } else if constexpr (Token.type == token_t::type_t::masked) {
                return mask::masked<Token.byte, Token.mask>{};
            } else {
                return mask::any{};
            }
        }

        template <size_t N, std::array<token_t, N> Tokens>
        consteval auto unwrapTokens() {
            return []<size_t... I>(std::index_sequence<I...>) {
                return std::tuple{unwrapToken<Tokens[I]>()...};
            }(std::make_index_sequence<N>());
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

    inline std::string to_string(token_t token) {
        switch (token.type) {
            case token_t::type_t::byte: return utils::hex_to_string(token.byte);
            case token_t::type_t::wildcard: return "?";
            case token_t::type_t::cursor: return "^";
            case token_t::type_t::masked: {
                std::string str;
                str.reserve(6);
                str += utils::hex_to_string(token.byte);
                str += '&';
                str += utils::hex_to_string(token.mask);
                return str;
            } break;
            default: return "";
        }
    }

    inline std::string to_string(std::span<const token_t> tokens) {
        std::string str;
        str.reserve(tokens.size() * 4);
        for (const auto& token : tokens) {
            switch (token.type) {
                case token_t::type_t::byte:
                    str += utils::hex_to_string(token.byte);
                    break;
                case token_t::type_t::wildcard:
                    str += "? ";
                    break;
                case token_t::type_t::cursor:
                    str += "^ ";
                    break;
                case token_t::type_t::masked:
                    str += utils::hex_to_string(token.byte);
                    str += '&';
                    str += utils::hex_to_string(token.mask);
                    break;
                default: break;
            }
        }

        // remove trailing space
        if (!str.empty()) {
            str.pop_back();
        }

        return str;
    }
}

#endif // SINAPS_PATTERN_HPP
