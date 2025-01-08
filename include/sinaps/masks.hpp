#pragma once
#ifndef SINAPS_MASKS_HPP
#define SINAPS_MASKS_HPP

#include <cstdint>
#include <tuple>
#include "token.hpp"
#include "utils.hpp"

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
}

#endif // SINAPS_MASKS_HPP
