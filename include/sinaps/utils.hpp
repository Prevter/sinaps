#pragma once
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
