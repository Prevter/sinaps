#pragma once
#include <array>
#include <cstdint>
#include <cstring>

#include "sinaps/masks.hpp"
#include "sinaps/pattern.hpp"

namespace sinaps {
    constexpr intptr_t not_found = -1;

    /// @brief Find an index of a pattern in a data buffer.
    /// The pattern is a sequence of masks (see <c>sinaps::masks</c> namespace).
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <typename Pattern> requires utils::is_specialization<Pattern, pattern>::value
    constexpr intptr_t find(const uint8_t* data, size_t size) {
        using pat = Pattern;

        for (size_t i = 0; i <= size - pat::size; i++) {
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
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <typename... Mask> requires (!utils::is_specialization<Mask, pattern>::value && ...)
    constexpr intptr_t find(const uint8_t* data, size_t size) {
        return find<pattern<Mask...>>(data, size);
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is an array of tokens.
    /// This method is a helper for the other find methods, use if you know what you're doing.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <std::array pattern> requires (pattern.size() > 0)
    constexpr intptr_t find(const uint8_t* data, size_t size) {
        return find<impl::make_pattern<pattern>>(data, size);
    }

    /// @brief Find an index of a pattern in a data buffer.
    /// Builds the pattern from a string literal.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <utils::FixedString S>
    constexpr intptr_t find(const uint8_t* data, size_t size) {
        return find<impl::tokenizePatternString<S>()>(data, size);
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is a list of tokens.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param pattern The pattern to search for.
    /// @param pattern_size Amount of tokens in the pattern.
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    constexpr intptr_t find(const uint8_t* data, size_t size, const token_t* pattern, size_t pattern_size) {
        for (size_t i = 0; i < size - pattern_size; i++) {
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
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    template <size_t N>
    constexpr intptr_t find(const uint8_t* data, size_t size, std::array<token_t, N> const& pattern) {
        return find(data, size, pattern.data(), N);
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is a list of tokens.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param pattern The pattern to search for.
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    constexpr intptr_t find(const uint8_t* data, size_t size, std::span<const token_t> pattern) {
        return find(data, size, pattern.data(), pattern.size());
    }

    /// @brief Find an index of a pattern in a data buffer. Pattern is a list of tokens.
    /// @param data The data buffer to search in.
    /// @param size The size of the data buffer.
    /// @param pattern The pattern to search for.
    /// @return The index of the pattern in the data buffer, or <b>sinaps::not_found</b> if not found.
    constexpr intptr_t find(const uint8_t* data, size_t size, std::initializer_list<token_t> pattern) {
        return find(data, size, pattern.begin(), pattern.size());
    }
}
