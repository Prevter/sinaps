#pragma once
#include <array>
#include <tuple>
#include <variant>

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

        for (size_t i = 0; i < size - pat::size; i++) {
            bool found = true;
            for (size_t j = 0; j < pat::size; j++) {
                if (pat::types[j] == token_t::type_t::byte && data[i + j] != pat::bytes[j]) {
                    found = false;
                    break;
                }
            }
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
                if (pattern[j].type == token_t::type_t::byte && data[i + j] != pattern[j].byte) {
                    found = false;
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
