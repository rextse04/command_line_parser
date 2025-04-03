#pragma once
#include <array>
#include "parser.hpp"

namespace cmd {
#define CONFIG_DEFAULT(CharT, CharT_prefix, FmtCharT, FmtCharT_prefix)\
    template <>\
    struct config_default<CharT, FmtCharT> {\
        using format_string_type = std::basic_string_view<FmtCharT>;\
        static constexpr format_string_type man_tmpl =\
            FmtCharT_prefix##"{0}\n{1}\n\033[36mUsages:\033[0m\n{2}\n{3}";\
        static constexpr format_string_type error_tmpl =\
            FmtCharT_prefix##"\033[31mError:\033[0m {0}{3}\n{1}\033[36mClosest usages:\033[0m\n{2}";\
        static constexpr format_string_type ref_tmpl =\
            FmtCharT_prefix##"| {}\n";\
        static constexpr special_chars<CharT> specials = {\
            CharT_prefix##' ', CharT_prefix##'\n', CharT_prefix##'"', CharT_prefix##'"', CharT_prefix##'\\',\
            CharT_prefix##'^',\
            CharT_prefix##"(", CharT_prefix##")", CharT_prefix##"|",\
            CharT_prefix##"[", CharT_prefix##"]", CharT_prefix##"-",\
            CharT_prefix##"<", CharT_prefix##">", CharT_prefix##"...", CharT_prefix##"="\
        };\
        static constexpr std::array<format_string_type, error_types_n> error_msgs = {\
            FmtCharT_prefix##"Unknown option.",\
            FmtCharT_prefix##"Too many arguments.",\
            FmtCharT_prefix##"Too few arguments.",\
            FmtCharT_prefix##"A flag cannot be assigned to a variable.",\
            FmtCharT_prefix##"Unknown flag.",\
            FmtCharT_prefix##"Flag does not accept argument.",\
            FmtCharT_prefix##"At least one special character is still open.",\
            FmtCharT_prefix##"Invalid argument:"\
        };\
    };
    CONFIG_DEFAULT(char, , char, )
    CONFIG_DEFAULT(wchar_t, L, wchar_t, L)
#undef CONFIG_DEFAULT
}