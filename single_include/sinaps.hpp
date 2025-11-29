#pragma once
#ifndef SINAPS_SINGLE_HEADER
#define SINAPS_SINGLE_HEADER

#ifndef SINAPS_UTILS_HPP
#define SINAPS_UTILS_HPP

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

namespace sinaps::utils {
    /// @brief A fixed-size string class.
    template <size_t N>
    struct [[nodiscard]] FixedString {
        char data[N]{};

        static constexpr void throw_if_invalid_index(size_t index) {
            index >= N ? throw std::out_of_range("Invalid index") : 0;
        }

        constexpr FixedString() = default;

        constexpr FixedString(const char (&str)[N]) {
            std::copy_n(str, N, data);
        }

        constexpr char& operator[](size_t index) {
            throw_if_invalid_index(index);
            return data[index];
        }

        constexpr const char& operator[](size_t index) const {
            throw_if_invalid_index(index);
            return data[index];
        }

        [[nodiscard]] constexpr auto tuple() const {
            return [this]<size_t... I>(std::index_sequence<I...>) {
                return std::tuple{static_cast<uint8_t>(data[I])...};
            }(std::make_index_sequence<N - 1>());
        }

        static constexpr size_t size() { return N - 1; }
        constexpr operator std::string_view() const { return std::string_view(data, N - 1); }
        constexpr operator std::string() const { return std::string(data, N); }
    };

    constexpr bool is_hex(char c) {
        return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
    }

    constexpr uint8_t from_hex(char c) {
        return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c - 'a' + 10;
    }

    template <size_t N, typename T>
    consteval auto tuple_repeat(T t) {
        if constexpr (N == 0) {
            return std::tuple{};
        } else {
            return std::tuple_cat(t, tuple_repeat<N - 1>(t));
        }
    }

    static constexpr FixedString<3> hex_to_string(uint8_t byte) {
        FixedString<3> str;
        constexpr char hex[] = "0123456789ABCDEF";
        str[0] = hex[byte >> 4];
        str[1] = hex[byte & 0xF];
        return str;
    }

    template <class, template <class...> class>
    struct is_specialization : std::false_type {};

    template <template <class...> class Template, class... Args>
    struct is_specialization<Template<Args...>, Template> : std::true_type {};
}

#endif // SINAPS_UTILS_HPP

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

#ifndef SINAPS_MASKS_HPP
#define SINAPS_MASKS_HPP

#include <cstdint>
#include <tuple>

namespace sinaps::mask {
    /// @brief A mask that matches a 64-bit value (8 bytes).
    template <uint64_t N>
    struct qword {
        static constexpr size_t size = 8;
        static constexpr std::tuple value = {
            token_t(N & 0xFF),
            token_t(N >> 8 & 0xFF),
            token_t(N >> 16 & 0xFF),
            token_t(N >> 24 & 0xFF),
            token_t(N >> 32 & 0xFF),
            token_t(N >> 40 & 0xFF),
            token_t(N >> 48 & 0xFF),
            token_t(N >> 56)
        };
    };

    /// @brief A mask that matches a 32-bit value (4 bytes).
    template <uint32_t N>
    struct dword {
        static constexpr size_t size = 4;
        static constexpr std::tuple value = {
            token_t(N & 0xFF),
            token_t(N >> 8 & 0xFF),
            token_t(N >> 16 & 0xFF),
            token_t(N >> 24)
        };
    };

    /// @brief A mask that matches a 16-bit value (2 bytes).
    template <uint16_t N>
    struct word {
        static constexpr size_t size = 2;
        static constexpr std::tuple value = {
            token_t(N & 0xFF),
            token_t(N >> 8)
        };
    };

    /// @brief A mask that matches a single byte.
    template <uint8_t N>
    struct byte {
        static constexpr size_t size = 1;
        static constexpr std::tuple value = {token_t(N)};
    };

    /// @brief A mask that matches any byte (wildcard).
    /// @tparam N The number of bytes to match.
    template <uint8_t N = 1>
    struct any {
        static constexpr size_t size = N;
        static constexpr std::tuple value = utils::tuple_repeat<N>(std::tuple{token_t()});
    };

    /// @brief A mask that matches a string literal.
    template <utils::FixedString S>
    struct string {
        static constexpr size_t size = S.size();

        template <size_t I>
        static constexpr auto getTupleImpl() {
            if constexpr (I == 0) {
                return std::tuple{};
            } else {
                return std::tuple_cat(getTupleImpl<I - 1>(), std::tuple{token_t(S[I - 1])});
            }
        }

        static constexpr std::tuple value = getTupleImpl<size>();
    };

    /// @brief A mask that allows for a sequence of masks to be repeated N times.
    template <size_t N, typename... Mask>
    struct sequence {
        static constexpr size_t size = (Mask::size + ...) * N;
        static constexpr std::tuple seqValue = std::tuple_cat(Mask::value...);
        static constexpr std::tuple value = utils::tuple_repeat<N>(seqValue);
    };

    /// @brief Special mask, that tells the origin index of the pattern.
    /// <c>sinaps::find</c> will return the index where this mask is found.
    struct cursor {
        static constexpr size_t size = 0;
        static constexpr std::tuple value = {token_t(token_t::type_t::cursor)};
    };

    /// @brief A mask that matches a byte with a mask.
    /// The mask is used to ignore certain bits in the byte.
    template <uint8_t N, uint8_t M>
    struct masked {
        static constexpr size_t size = 1;
        static constexpr std::tuple value = {token_t(N, M)};
    };
}

#endif // SINAPS_MASKS_HPP

#ifndef SINAPS_PATTERN_HPP
#define SINAPS_PATTERN_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>


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

        // count number of groups of consecutive tokens (e.g. "AB?^C" has 2 groups)
        static constexpr size_t group_count = []<size_t... I>(std::index_sequence<I...>) {
            size_t count = 0;
            bool in_group = false;
            ((std::get<I>(types) == token_t::type_t::byte
                  ? (void) (in_group = true)
                  : in_group
                        ? (void) (in_group = false, count++)
                        : void()), ...);
            if (in_group) count++;
            return count;
        }(std::make_index_sequence<size>());

        struct group_t {
            size_t offset{};
            size_t count{};
            constexpr group_t() = default;
            constexpr group_t(size_t offset, size_t count) : offset(offset), count(count) {}
            [[nodiscard]] constexpr size_t size() const { return count; }
            constexpr uint8_t* begin(uint8_t* data) const { return data + offset; }
            constexpr uint8_t* end(uint8_t* data) const { return data + offset + count; }
        };

        // array of groups
        static constexpr std::array<group_t, group_count> groups = []<size_t... I>(std::index_sequence<I...>) {
            std::array<group_t, group_count> groups;
            size_t index = 0;
            size_t begin = 0;
            size_t count = 0;
            bool in_group = false;
            ((std::get<I>(types) == token_t::type_t::byte
                  ? in_group
                        ? void(count++)
                        : (void) (in_group = true, count = 1, begin = I)
                  : in_group
                        ? (void) (in_group = false, groups[index++] = group_t{begin, count})
                        : void()), ...);
            if (in_group) groups[index] = group_t{begin, count};
            return groups;
        }(std::make_index_sequence<size>());

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

        template <auto Tokens, size_t... I>
        consteval auto unwrapTokens(std::index_sequence<I...>) {
            return std::tuple{unwrapToken<Tokens[I]>()...};
        }

        /// @brief Get a pattern type from a list of tokens.
        template <std::array P>
        using make_pattern = decltype(pattern{unwrapTokens<P>(std::make_index_sequence<P.size()>())});
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
                    str += ' ';
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
                    str += ' ';
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

#include <array>
#include <cstdint>
#include <cstring>


#ifndef SINAPS_RESTRICT
    #if defined(__GNUC__) || defined(__clang__)
        #define SINAPS_RESTRICT __attribute__((restrict))
    #elif defined(_MSC_VER)
        #define SINAPS_RESTRICT __restrict
    #else
        #define SINAPS_RESTRICT
    #endif
#endif

#ifndef SINAPS_HOT
    #if defined(__GNUC__) || defined(__clang__)
        #define SINAPS_HOT [[gnu::hot]]
    #else
        #define SINAPS_HOT
    #endif
#endif

namespace sinaps {
    constexpr intptr_t not_found = -1;

    /// @brief Find an index of a pattern in a data buffer.
    /// The pattern is a sequence of masks (see <c>sinaps::masks</c> namespace).
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <typename Pattern> requires utils::is_specialization<Pattern, pattern>::value
    SINAPS_HOT constexpr intptr_t find(uint8_t const* SINAPS_RESTRICT data, size_t size, size_t step_size = 1) {
        using pat = Pattern;

        for (size_t i = 0; i <= size - pat::size; i += step_size) {
            bool found = true;

            // check for groups
            for (auto& group : pat::groups) {
                if (std::is_constant_evaluated()) {
                    // memcmp is not allowed in consteval
                    for (size_t j = 0; j < group.count; j++) {
                        if (data[i + j + group.offset] != pat::bytes[j + group.offset]) {
                            found = false;
                            break;
                        }
                    }
                } else {
                    auto* data_ptr = data + i + group.offset;
                    auto* pattern_ptr = pat::bytes.data() + group.offset;

                    if (std::memcmp(data_ptr, pattern_ptr, group.count) != 0) {
                        found = false;
                        break;
                    }
                }
            }

            if (found) {
                // check for masked bytes
                for (size_t j = 0; j < pat::size; j++) {
                    if (pat::types[j] == token_t::type_t::masked && (data[i + j] & pat::masks[j]) != pat::bytes[j]) {
                        found = false;
                        break;
                    }
                }
            }

            // check if we found the pattern
            if (found) {
                return i + pat::cursor_pos;
            }
        }

        // not found
        return not_found;
    }

    /// @brief Find an index of a pattern in a data buffer.
    /// The pattern is a sequence of masks (see <c>sinaps::masks</c> namespace).
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <typename... Mask> requires (!utils::is_specialization<Mask, pattern>::value && ...)
    constexpr intptr_t find(uint8_t const* data, size_t size, size_t step_size = 1) {
        return find<pattern<Mask...>>(data, size, step_size);
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is an array of tokens.
    /// This method is a helper for the other find methods, use if you know what you're doing.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <std::array pattern> requires (pattern.size() > 0)
    constexpr intptr_t find(uint8_t const* data, size_t size, size_t step_size = 1) {
        return find<impl::make_pattern<pattern>>(data, size, step_size);
    }

    /// @brief Find an index of a pattern in a data buffer.
    /// Builds the pattern from a string literal.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <utils::FixedString S>
    constexpr intptr_t find(uint8_t const* data, size_t size, size_t step_size = 1) {
        return find<impl::tokenizePatternString<S>()>(data, size, step_size);
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is a list of tokens.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param pattern The pattern to search for.
    /// @param pattern_size Amount of tokens in the pattern.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    SINAPS_HOT constexpr intptr_t find(uint8_t const* SINAPS_RESTRICT data, size_t size, token_t const* SINAPS_RESTRICT pattern, size_t pattern_size, size_t step_size = 1) {
        for (size_t i = 0; i < size - pattern_size; i += step_size) {
            bool found = true;
            for (size_t j = 0; j < pattern_size; j++) {
                switch (pattern[j].type) {
                    case token_t::type_t::byte:
                        if (data[i + j] != pattern[j].byte) {
                            found = false;
                            break;
                        }
                        break;
                    case token_t::type_t::masked:
                        if ((data[i + j] & pattern[j].mask) != pattern[j].byte) {
                            found = false;
                            break;
                        }
                        break;
                    default:
                        break;
                }
            }
            if (found) {
                return i;
            }
        }

        // not found
        return not_found;
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is a list of tokens.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param pattern The pattern to search for.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <size_t N>
    constexpr intptr_t find(uint8_t const* data, size_t size, std::array<token_t, N> const& pattern, size_t step_size = 1) {
        return find(data, size, pattern.data(), N, step_size);
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is a list of tokens.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param pattern The pattern to search for.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    constexpr intptr_t find(uint8_t const* data, size_t size, std::span<const token_t> pattern, size_t step_size = 1) {
        return find(data, size, pattern.data(), pattern.size(), step_size);
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is a list of tokens.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param pattern The pattern to search for.
    /// @param step_size The step size for the search (default is 1).
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    constexpr intptr_t find(uint8_t const* data, size_t size, std::initializer_list<token_t> pattern, size_t step_size = 1) {
        return find(data, size, pattern.begin(), pattern.size(), step_size);
    }
}

#endif // SINAPS_SINGLE_HEADER
