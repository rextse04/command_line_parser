#pragma once
#include <iostream>
#include "common.hpp"

namespace cmd {
    template <typename CharT>
    concept char_like = requires {
        requires std::equality_comparable<CharT>;
    };

    /// If a specialization of \code output_object\endcode sets \code enabled\endcode to \code true\endcode,
    /// it must supply a (global) \code std::basic_ostream<CharT>\endcode for a \code parser\endcode to write to.
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
        requires std::formattable<std::basic_string_view<CharT>, FmtCharT>;
    };

    /// If a specialization of \code input_object\endcode sets \code enabled\endcode to \code false\endcode,
    /// it must supply a (global) \code std::basic_istream<CharT>\endcode for a \code parser\endcode to read from.
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