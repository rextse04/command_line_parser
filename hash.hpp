#pragma once
#include <string_view>
#include "common.hpp"

namespace cmd {
    // same specifications as std::hash, except operator() must be constexpr
    template <typename T>
    struct hash {};

    namespace {
        constexpr size_t polyhash_base = 13; // usually prime
    }

    template <typename T>
    requires (
        is_template_instance_v<std::basic_string_view, T> &&
        requires (T::value_type c, size_t num) {{c * num} -> std::convertible_to<size_t>;}
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