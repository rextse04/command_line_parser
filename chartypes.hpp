#pragma once
#include <iostream>
#include <locale>
#include "common.hpp"

namespace cmd {
    template <typename CharT>
    concept char_like = requires {
        requires std::is_trivial_v<CharT>;
        requires std::is_standard_layout_v<CharT>;
        requires std::equality_comparable<CharT>;
    };

    /// If a specialization of \code output_object\endcode sets \code enabled\endcode to \code true\endcode,
    /// it must supply a (global) \code std::basic_ostream<CharT>\endcode for a \code parser\endcode to write to.
    template <char_like>
    struct output_object {
        static constexpr auto value = nullptr;
    };
    template <>
    struct output_object<char> {
        static constexpr auto value = &std::cout;
    };
    template <>
    struct output_object<wchar_t> {
        static constexpr auto value = &std::wcout;
    };

    /// If a specialization of \code input_object\endcode sets \code enabled\endcode to \code false\endcode,
    /// it must supply a (global) \code std::basic_istream<CharT>\endcode for a \code parser\endcode to read from.
    template <char_like>
    struct input_object {
        static constexpr auto value = nullptr;
    };
    template <>
    struct input_object<char> {
        static constexpr auto value = &std::cin;
    };
    template <>
    struct input_object<wchar_t> {
        static constexpr auto* value = &std::wcin;
    };

    /// Based on `std::codecvt<InCharT, OutCharT, std::mbstate_t>`.\n
    /// If `reverse` is enabled, `InCharT` and `OutCharT` is swapped.
    template <char_like InCharT, char_like OutCharT, bool reverse = false>
    struct translator {
        using converter_type = std::conditional_t<reverse,
            std::codecvt<OutCharT, InCharT, std::mbstate_t>,
            std::codecvt<InCharT, OutCharT, std::mbstate_t>>;
        static constexpr std::size_t default_size_mul = sizeof(InCharT) / sizeof(OutCharT) + 1;
        /// \param in: input string
        /// \param size_mul: size of output buffer in multiples of `in.size`
        /// \param locale: locale that the conversion should be based on
        /// (if not specified, the global locale is used)
        /// \throw std::bad_cast if conversion facet does not exist
        /// \throw std::runtime_error if there is any error during conversion
        std::basic_string<OutCharT> operator()
        (std::basic_string_view<InCharT> in, std::size_t size_mul = default_size_mul, const std::locale& locale = {}) const {
            std::basic_string<OutCharT> out(in.size() * size_mul, 0);
            const auto& facet = std::use_facet<converter_type>(locale);
            std::mbstate_t mb{};
            const InCharT* from_next;
            OutCharT* to_next;
            std::codecvt_base::result res;
            if constexpr (reverse) {
                res = facet.in(mb, in.data(), in.data() + in.size(), from_next,
                    out.data(), out.data() + out.size(), to_next);
            } else {
                res = facet.out(mb, in.data(), in.data() + in.size(), from_next,
                    out.data(), out.data() + out.size(), to_next);
            }
            switch (res) {
                using enum std::codecvt_base::result;
                case partial: {
                    throw std::runtime_error("insufficient buffer size");
                }
                case error: {
                    throw std::runtime_error("invalid character encountered");
                }
                case ok: {
                    break;
                }
                default: {
                    std::unreachable();
                }
            }
            out.resize(to_next - out.data());
            return out;
        }
    };
    template <char_like CharT, bool reverse>
    struct translator<CharT, CharT, reverse> {
        constexpr auto operator()(std::basic_string_view<CharT> in, const std::locale& locale = {}) const noexcept {
            return in;
        }
    };
}