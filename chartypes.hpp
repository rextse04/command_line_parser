#pragma once
#include <iostream>
#include "common.hpp"

namespace cmd {
    template <typename CharT>
    concept char_like = requires {
        requires std::equality_comparable<CharT>;
    };

    template <char_like>
    struct output_object {
        static constexpr bool enabled = false;
    };
    template <>
    struct output_object<char> {
        static constexpr bool enabled = true;
        static constexpr auto& value = std::cout;
    };
    template <>
    struct output_object<wchar_t> {
        static constexpr bool enabled = true;
        static constexpr auto& value = std::wcout;
    };
    template <typename CharT, typename FmtCharT>
    concept outputtable = requires {
        requires output_object<CharT>::enabled;
        requires std::formattable<std::basic_string_view<CharT>, FmtCharT>;
    };

    template <char_like>
    struct input_object {
        static constexpr bool enabled = false;
    };
    template <>
    struct input_object<char> {
        static constexpr bool enabled = true;
        static constexpr auto& value = std::cin;
    };
    template <>
    struct input_object<wchar_t> {
        static constexpr bool enabled = true;
        static constexpr auto& value = std::wcin;
    };
}