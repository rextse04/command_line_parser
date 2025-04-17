#pragma once
#include <vector>
#include <string_view>
#include <charconv>
#include "common.hpp"

namespace cmd::detail {
    constexpr std::size_t define_error_buffer_size = 1024,
    index_str_buffer_size = sizeof(std::size_t) * CHAR_BIT / 3; // overestimation
    enum class define_error_ref_type {
        error, warning, note
    };
    struct define_error_ref {
        std::string_view what;
        std::size_t usage_index, loc;
        define_error_ref_type type = define_error_ref_type::error;
    };
    struct define_error {
    private:
        char buffer_[define_error_buffer_size]{};
        std::size_t size_;
    public:
        template <typename ConfigT>
        constexpr define_error(const ConfigT& config, const auto& refs) noexcept {
            using enum define_error_ref_type;
            char *current = buffer_, *const end = buffer_ + define_error_buffer_size;
            auto append = [this, &current](std::string_view str) {
                current = ranges::copy(str, current).out;
            };
            auto append_n_spaces = [this, &current](std::size_t n) {
                for (std::size_t i = 0; i < n; ++i) {
                    current[i] = ' ';
                }
                current += n;
            };
            append("Parse error.\n");
            std::vector<std::pair<char[index_str_buffer_size], std::size_t>> index_strs;
            index_strs.resize(ranges::size(refs));
            std::size_t index_str_max_size = 0;
            for (auto [ref, index_str]: views::zip(refs, index_strs)) {
                index_str.second =
                    std::to_chars(index_str.first, index_str.first + index_str_buffer_size, ref.usage_index).ptr -
                    index_str.first;
                if (index_str.second > index_str_max_size) {
                    index_str_max_size = index_str.second;
                }
            }
            for (auto [ref, index_str]: views::zip(refs, index_strs)) {
                switch (ref.type) {
                    case error: {
                        append("\033[31mError: \033[0m");
                        break;
                    }
                    case warning: {
                        append("\033[33mWarning: \033[0m");
                        break;
                    }
                    case note: {
                        append("\033[36mNote: \033[0m");
                        break;
                    }
                }
                append(ref.what);
                if constexpr (std::is_same_v<typename ConfigT::super_type::char_type, char>) {
                    append("\n");
                    append_n_spaces(index_str_max_size - index_str.second + 1);
                    append({index_str.first, index_str.second});
                    append(" | ");
                    append(config.usages[ref.usage_index].format);
                    append("\n");
                    append_n_spaces(index_str_max_size + 2);
                    append("| ");
                    append_n_spaces(ref.loc);
                    append("^\n");
                } else {
                    append(" (in usage no. ");
                    current = std::to_chars(current, end, ref.usage_index).ptr;
                    append(")\n");
                }
            }
            size_ = current - buffer_ - 1;
        }
        [[nodiscard]] constexpr const char* data() const noexcept {
            return buffer_;
        }
        [[nodiscard]] constexpr std::size_t size() const noexcept {
            return size_;
        }
    };
}