#pragma once
#include <string_view>
#include "common.hpp"

namespace cmd {
    /// Specialize this template to add a default hasher.
    template <typename>
    struct hash {};

    namespace {
        constexpr size_t polyhash_base = 13; // usually prime
    }

    /// Default hasher for string views of arithmetic \code char_type\endcode.
    template <typename T>
    requires (
        is_template_instance_v<std::basic_string_view, T> &&
        requires (typename T::value_type c, size_t num) {{c * num} -> std::convertible_to<std::size_t>;}
    )
    struct hash<T> {
        constexpr size_t operator()(T s) const noexcept {
            size_t out = 0, term = 1;
            for (auto c : s) {
                out += c * term;
                term *= polyhash_base;
            }
            return out;
        }
    };
}