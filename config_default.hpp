#pragma once
#include <array>
#include "parser.hpp"

namespace cmd {
/// \param CharT: char_type
/// \param CharT_prefix: string literal prefix corresponding to CharT
/// \param FmtCharT: format_char_type
/// \param FmtCharT_prefix: string literal prefix corresponding to FmtCharT
#define CONFIG_DEFAULT(CharT, CharT_prefix, FmtCharT, FmtCharT_prefix)\
    template <>\
    struct config_default<CharT, FmtCharT> {\
        using format_string_type = std::basic_string_view<FmtCharT>;\
        static constexpr format_string_type man_tmpl =\
            FmtCharT_prefix##"\033[4m{0}\033[0m\n{1}\n\033[36mUsages:\033[0m\n{2}{3}";\
        static constexpr format_string_type error_tmpl =\
            FmtCharT_prefix##"\033[31mError:\033[0m {0}{3}\n{1}\033[36mClosest usages:\033[0m\n{2}";\
        static constexpr format_string_type usage_tmpl =\
            FmtCharT_prefix##"| {0}\n";\
        static constexpr special_chars<CharT> specials = {\
            CharT_prefix##' ', CharT_prefix##'\n', CharT_prefix##'"', CharT_prefix##'"', CharT_prefix##'\\',\
            CharT_prefix##'^',\
            CharT_prefix##"(", CharT_prefix##")", CharT_prefix##"|",\
            CharT_prefix##"[", CharT_prefix##"]", CharT_prefix##"-",\
            CharT_prefix##"<", CharT_prefix##">", CharT_prefix##"...", CharT_prefix##"=", CharT_prefix##"..."\
        };\
        static constexpr std::array<format_string_type, error_types_n> error_msgs = {\
            FmtCharT_prefix##"Unknown option.",\
            FmtCharT_prefix##"Too many arguments.",\
            FmtCharT_prefix##"Too few arguments.",\
            FmtCharT_prefix##"A flag cannot be assigned to a variable.",\
            FmtCharT_prefix##"Unknown flag.",\
            FmtCharT_prefix##"Flag does not accept argument.",\
            FmtCharT_prefix##"At least one special character is still open.",\
            FmtCharT_prefix##"Invalid argument: "\
        };\
    };
    CONFIG_DEFAULT(char, , char, )
    CONFIG_DEFAULT(wchar_t, L, char, )
    CONFIG_DEFAULT(char8_t, u8, char, )
    CONFIG_DEFAULT(char16_t, u, char, )
    CONFIG_DEFAULT(char32_t, U, char, )
    CONFIG_DEFAULT(char, , wchar_t, L)
    CONFIG_DEFAULT(wchar_t, L, wchar_t, L)
    CONFIG_DEFAULT(char8_t, u8, wchar_t, L)
    CONFIG_DEFAULT(char16_t, u, wchar_t, L)
    CONFIG_DEFAULT(char32_t, U, wchar_t, L)
}