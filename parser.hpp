#pragma once
#include <array>
#include <algorithm>
#include <string>
#include <bitset>
#include <vector>
#include <expected>
#include <iostream>
#include <climits>
#include <complex>
#include <utility>
#include "common.hpp"
#include "chartypes.hpp"
#include "hash.hpp"

namespace cmd {
    // unknown error must be the last
    // also used as index
    enum class error_type {
        unknown_option,
        too_many_arguments,
        too_few_arguments,
        flag_cannot_be_variable,
        unknown_flag,
        flag_does_not_accept_argument,
        open_special_character,
        invalid_argument,
        unknown_error
    };
    constexpr std::size_t error_types_n = std::to_underlying(error_type::unknown_error) + 1;

    template <typename T>
    concept usage_id = std::is_integral_v<T> || std::is_enum_v<T>;
    struct usage_tag;
    template <usage_id Result, char_like CharT>
    struct usage {
        using tag = usage_tag;
        using result_type = Result;
        using char_type = CharT;
        using string_type = std::basic_string_view<char_type>;
        string_type format;
        Result name;
    };

    struct usage_range_tag;
    template <auto& FormatString>
    requires (
        is_template_instance_v<std::basic_string_view, std::remove_cvref_t<decltype(FormatString)>> &&
        char_like<typename std::remove_cvref_t<decltype(FormatString)>::value_type>
    )
    struct usage_range {
        template <typename Rng>
        requires (ranges::input_range<Rng> && tagged<ranges::range_value_t<Rng>, usage_tag>)
        struct type {
            using tag = usage_range_tag;
            using char_type = ranges::range_value_t<Rng>::char_type;
            using format_string_type = std::remove_cvref_t<decltype(FormatString)>;
            using format_char_type = format_string_type::value_type;
            static constexpr const format_string_type& format = FormatString;
            const Rng& value;
        };
    };

    template <char_like CharT>
    struct special_chars {
        CharT delimiter, enter, quote_open, quote_close, escape, indicator;
        std::basic_string_view<CharT> compound_open , compound_close, compound_divider,
            flag_open, flag_close, flag_prefix,
            var_open, var_close, equal;
    };
    struct config_tag;
    template <char_like CharT, char_like FmtCharT>
    struct config_default;
    template <usage_id Result, char_like CharT = char, char_like FmtCharT = char>
    requires outputtable<CharT, FmtCharT>
    struct config {
        using tag = config_tag;
        using result_type = Result;
        using char_type = CharT;
        using format_char_type = FmtCharT;
        using string_type = std::basic_string_view<char_type>;
        using format_string_type = std::basic_string_view<format_char_type>;
        using config_default_type = config_default<char_type, format_char_type>;
        template <size_t N>
        struct type {
            using super_type = config;
            string_type name, description;
            usage<result_type, char_type> usages[N];
            string_type explanation;
            format_string_type man_tmpl = config_default_type::man_tmpl;
            format_string_type error_tmpl = config_default_type::error_tmpl;
            format_string_type ref_tmpl = config_default_type::ref_tmpl;
            special_chars<char_type> specials = config_default_type::specials;
            std::array<format_string_type, error_types_n> error_msgs = config_default_type::error_msgs;
        };
    };
    template <typename T>
    concept config_instance = tagged<typename T::super_type, config_tag>;

    enum class parse_node_type {
        option, variable, end
    };
    template <char_like CharT>
    struct parse_node {
        parse_node_type type;
        union {
            std::basic_string_view<CharT> option_name{};
            std::size_t var_index;
            std::size_t usage_index;
        };
        std::size_t next_placeholder = 0; // 0: no next placeholder
        std::size_t next = 0; // 0: invalid
    };

    template <size_t UsageSize, char_like CharT>
    struct flag_info {
        std::basic_string_view<CharT> name;
        std::bitset<UsageSize> defined_for;
        std::array<size_t, UsageSize> var_index_for{}; // var_index = 0: not defined
        constexpr bool defined() const noexcept {
            return defined_for.any();
        }
    };

    template <typename Hash, typename CharT>
    concept hasher = requires(std::basic_string_view<CharT> str) {
        {Hash{}(str)} -> std::same_as<size_t>;
    };
    template <typename Hash, std::size_t SetSize>
    static constexpr std::size_t get_hash(auto str) noexcept {
        return Hash{}(str) % SetSize;
    }

    struct parser_def_tag;
    template <config_instance Config>
    struct parser_def {
        using tag = parser_def_tag;
        std::size_t usage_size, tree_size, vars_size, flag_set_size, var_num;
        const Config& config;
    };

    struct parser_info_tag;
    template <usage_id Result, char_like CharT, hasher<CharT> Hash, tagged<parser_def_tag> auto Def>
    struct parser_info {
        using tag = parser_info_tag;
        using result_type = Result;
        using char_type = CharT;
        using hash_type = Hash;
        using string_type = std::basic_string_view<char_type>;
        static constexpr auto def = Def;
        std::array<usage<Result, CharT>, Def.usage_size> usages;
        std::array<parse_node<CharT>, Def.tree_size> tree;
        std::array<string_type, Def.vars_size> var_names;
        std::array<flag_info<Def.usage_size, CharT>, Def.flag_set_size> flag_set;
    };

    struct error_loc {
        std::size_t arg_loc, in_arg_loc;
    };
    struct parse_result_tag;
    struct error_tag;
    struct error_ref_tag;
    struct error_ref_c_tag;
    template <tagged<parser_info_tag> auto& Info>
    class parser {
    public:
        static constexpr const auto& info = Info;
        static constexpr const auto& def = Info.def;
        static constexpr const auto& config = Info.def.config;
        using info_type = std::remove_cvref_t<decltype(Info)>;
        using def_type = std::remove_cvref_t<decltype(def)>;
        using config_type = std::remove_cvref_t<decltype(config)>;
        using result_type = info_type::result_type;
        using char_type = info_type::char_type;
        using string_type = std::basic_string_view<char_type>;
        using format_char_type = config_type::super_type::format_char_type;
        using hash_type = info_type::hash_type;
        using refs_type = std::vector<usage<result_type, char_type>>;
        static constexpr bool input_enabled = input_object<char_type>::enabled;
        static constexpr bool output_enabled = outputtable<char_type, format_char_type>;
        static constexpr auto& input_stream = input_object<char_type>::value;
        static constexpr auto& output_stream = output_object<format_char_type>::value;
    private:
        static constexpr auto to_string = [](const auto& obj) noexcept {
            return string_type{obj};
        };
        struct var_name {
            std::size_t index;
            consteval var_name(const char_type* name) noexcept {
                std::array<bool, Info.var_names.size()> match;
                for (std::size_t i = 1; i < match.size(); ++i) {
                    match[i] = (name == Info.var_names[i]);
                }
                auto search = ranges::find(match | views::drop(1), true);
                if (search == match.end()) {
                    throw std::invalid_argument("Unknown variable.");
                } else {
                    index = search - match.begin();
                }
            }
        };
        struct flag_name {
            std::size_t index;
            consteval flag_name(const char_type* name) noexcept {
                const std::size_t h = get_hash<hash_type, Info.flag_set.size()>(name);
                if (name == Info.flag_set[h].name) [[likely]] {
                    index = h;
                } else {
                    throw std::invalid_argument("Unknown flag. Note that preceding '-'(s) must be included.");
                }
            }
        };
        std::array<std::pair<string_type, error_loc>, Info.var_names.size()> vars_{}; // starts from 1
        std::bitset<Info.flag_set.size()> flags_{};
        template <int Mode>
        struct error_ref_wrapper {
            template <tagged<error_ref_tag> Ref>
            struct type {
                using tag = error_ref_c_tag;
                using super_type = parser;
                static constexpr int mode = Mode;
                const Ref& ref;
            };
        };
        template <typename Rng>
        struct error_ref {
            using tag = error_ref_tag;
            using super_type = parser;
            receiver_type_t<Rng> args;
            error_loc loc;
            explicit constexpr error_ref(Rng&& rng) :
                args{std::forward<Rng>(rng)}, loc{-1uz} {}
            constexpr error_ref(Rng&& rng, error_loc loc) :
                args{std::forward<Rng>(rng)}, loc{loc} {}
            template <int Mode>
            constexpr auto wrap() const noexcept {
                return typename error_ref_wrapper<Mode>::type{*this};
            }
        };
    public:
        template <typename Rng>
        struct parse_error {
            using tag = error_tag;
            using super_type = parser;
            error_type type;
            error_ref<Rng&&> ref;
            refs_type refs;

            constexpr parse_error(error_type type, Rng&& rng, refs_type refs = {}) noexcept :
                type{type}, ref{std::forward<Rng>(rng)}, refs{std::move(refs)} {}
            constexpr parse_error(error_type type, Rng&& rng, error_loc loc, refs_type refs = {}) noexcept :
                type{type}, ref{std::forward<Rng>(rng), loc}, refs{std::move(refs)} {}
            STALE_CLASS(parse_error)

            auto print(std::output_iterator<format_char_type> auto out) const
            requires output_enabled {
                return std::format_to(
                    out, config.error_tmpl, config.error_msgs[std::to_underlying(type)],
                    ref, typename usage_range<config.ref_tmpl>::type{refs}, string_type{}
                );
            }
            auto print() const
            requires output_enabled {
                return print(std::ostreambuf_iterator{output_stream});
            }
        };
        template <typename Rng>
        struct argument_error {
            using tag = error_tag;
            using super_type = parser;
            string_type what;
            error_ref<const Rng&> ref;
            std::size_t usage_index;

            constexpr argument_error(string_type what, const Rng& args, std::size_t usage_index) noexcept :
                what{what}, ref{args}, usage_index{usage_index} {}
            constexpr argument_error(
                string_type what, const Rng& args, error_loc loc, std::size_t usage_index) noexcept :
                what{what}, ref{args, loc}, usage_index{usage_index} {}
            STALE_CLASS(argument_error)

            auto print(std::output_iterator<format_char_type> auto out) const
            requires output_enabled {
                return std::format_to(
                    out, config.error_tmpl, config.error_msgs[std::to_underlying(error_type::invalid_argument)],
                    ref, typename usage_range<config.ref_tmpl>::type{std::array{Info.usages[usage_index]}}, what
                );
            }
            auto print() const
            requires output_enabled {
                return print(std::ostreambuf_iterator{output_stream});
            }
        };
        template <typename Rng>
        struct parse_result {
            using tag = parse_result_tag;
            using super_type = parser;
            result_type result;
            std::size_t usage_index;
            receiver_type_t<Rng&&> args;
            constexpr parse_result(result_type result, std::size_t usage_index, Rng&& args) :
                result{result}, usage_index{usage_index}, args{std::forward<Rng>(args)} {}
            STALE_CLASS(parse_result)
        };
    protected:
        static constexpr refs_type search_refs(std::size_t node_loc) noexcept {
            using enum parse_node_type;
            std::array<size_t, Info.tree.size()> deque;
            std::array<bool, Info.tree.size()> visited{};
            refs_type refs;
            auto front = deque.begin(), back = front + 1;
            *front = node_loc;
            auto push = [&back, &visited](std::size_t node_idx) {
                if (!visited[node_idx]) {
                    *(back++) = node_idx;
                    visited[node_idx] = true;
                }
            };
            while (front != back) {
                switch (Info.tree[*front].type) {
                    case option: {
                        std::size_t next_option = Info.tree[*front].next_placeholder;
                        if (next_option) push(next_option);
                        push(Info.tree[*front].next);
                        break;
                    }
                    case variable:
                        push(Info.tree[*front].next);
                        break;
                    case end:
                        refs.push_back(Info.usages[Info.tree[*front].usage_index]);
                }
                ++front;
            }
            return refs;
        }
    public:
        template <typename Rng>
        requires RANGE_OF(forward_range, string_type)
        constexpr auto parse(Rng&& args) noexcept {
            using enum parse_node_type;
            using enum error_type;
            using return_type = std::expected<parse_result<Rng>, parse_error<Rng>>;
            auto node = Info.tree.begin();
            auto start_node = node;
            auto arg_current = ranges::begin(args);
            auto arg_end = ranges::end(args);
            std::size_t arg_loc = 0;
            auto next_arg = [&start_node, &node, &arg_current, &arg_loc](std::size_t next_node_idx) {
                start_node = node = Info.tree.begin() + next_node_idx;
                ++arg_current; ++arg_loc;
            };
            auto raise = [&start_node, &args, &arg_loc](error_type err, std::size_t in_arg_loc = 0) {
                return return_type{std::unexpect,
                    err, std::forward<Rng>(args),
                    error_loc{arg_loc, in_arg_loc}, search_refs(start_node - Info.tree.begin())
                };
            };
            while (true) {
                if (arg_current == arg_end) [[unlikely]] {
                    while (node->next_placeholder) {
                        node = Info.tree.begin() + node->next_placeholder;
                    }
                    if (node->type != end) {
                        return raise(too_few_arguments);
                    }
                }
                switch (node->type) {
                    case option:
                        if (node->option_name == *arg_current) {
                            next_arg(node->next);
                        } else if (node->next_placeholder) {
                            node = Info.tree.begin() + node->next_placeholder;
                        } else [[unlikely]] {
                            return raise(unknown_option);
                        }
                        continue;
                    case variable:
                        if ((*arg_current).starts_with(config.specials.flag_prefix)) {
                            return raise(flag_cannot_be_variable);
                        }
                        vars_[node->var_index] = {*arg_current, {arg_loc, 0}};
                        next_arg(node->next);
                        continue;
                    case end:
                        break;
                }
                break;
            }
            for (;arg_current != arg_end; ++arg_current) {
                std::basic_string_view<char_type> flag_str = *arg_current;
                if (flag_str.starts_with(config.specials.flag_prefix)) [[likely]] {
                    const std::size_t eq_pos = flag_str.find(config.specials.equal);
                    std::basic_string_view<char_type> flag_name = flag_str.substr(0, eq_pos);
                    const std::size_t h = get_hash<hash_type, Info.flag_set.size()>(flag_name);
                    const auto& flag = Info.flag_set[h];
                    if (flag.name == flag_name && flag.defined_for[node->usage_index]) [[likely]] {
                        flags_[h] = true;
                        if (eq_pos != flag_str.npos) {
                            if (std::size_t var_index = flag.var_index_for[node->usage_index]) [[likely]] {
                                vars_[var_index] = {
                                    flag_str.substr(eq_pos + config.specials.equal.size()),
                                    {arg_loc, eq_pos + 1}
                                };
                            } else {
                                return raise(flag_does_not_accept_argument, eq_pos);
                            }
                        }
                    } else {
                        return raise(unknown_flag);
                    }
                } else {
                    return raise(too_many_arguments);
                }
            }
            return return_type{std::in_place,
                Info.usages[node->usage_index].name, node->usage_index, std::forward<Rng>(args)};
        }
        constexpr auto parse(int argc, const char_type** argv) noexcept {
            return parse(views::counted(argv + 1, argc - 1) | views::transform(to_string));
        }
        template <typename Iter>
        requires ITER_OF(input_iterator, char_type)
        constexpr auto parse(Iter it) {
            using enum error_type;
            std::vector<std::basic_string<char_type>> args;
            args.assign({{}});
            bool quote_open = false, escape = false;
            for (; *it != config.specials.enter; ++it) {
                if (quote_open || (*it != config.specials.delimiter)) {
                    if (escape) {
                        escape = false;
                    } else {
                        if constexpr (config.specials.quote_open == config.specials.quote_close) {
                            if (*it == config.specials.quote_open) {
                                quote_open = !quote_open;
                                continue;
                            }
                        } else {
                            if (*it == config.specials.quote_open) {
                                quote_open = true;
                                continue;
                            }
                            if (*it == config.specials.quote_close) {
                                quote_open = false;
                                continue;
                            }
                        }
                        if (*it == config.specials.escape) {
                            escape = true;
                            continue;
                        }
                    }
                    args.back().push_back(*it);
                } else {
                    if (!args.back().empty()) args.push_back({});
                }
            }
            if (args.front().empty()) args.clear();

            auto get_return = [this, &args]() {return parse(std::move(args) | views::transform(to_string));};
            using return_type = std::invoke_result_t<decltype(get_return)>;
            if (quote_open || escape) [[unlikely]] {
                return return_type{std::unexpect, open_special_character, std::move(args) | views::transform(to_string)};
            }
            return get_return();
        }
        auto parse()
        requires input_enabled {
            return parse(std::istreambuf_iterator<char_type>(input_stream));
        }
        template <typename Rng>
        constexpr argument_error<std::remove_reference_t<Rng>> raise_argument_error(
            const parse_result<Rng>& result, var_name name, string_type what) const noexcept {
            return {what, result.args, vars_[name.index].second, result.usage_index};
        }
        void print_man(std::output_iterator<char_type> auto out, const auto&... args) const
        requires output_enabled {
            std::format_to(
                out, config.man_tmpl,
                config.name, config.description,
                typename usage_range<config.ref_tmpl>::type{config.usages},
                config.explanation, args...
            );
        }
        void print_man(const auto&... args) const
        requires output_enabled {
            print_man(std::ostreambuf_iterator{output_stream}, args...);
        }
        constexpr std::basic_string_view<char_type> var(var_name name) const noexcept {
            return vars_[name.index].first;
        }
        constexpr bool flag(flag_name name) const noexcept {
            return flags_[name.index];
        }
        constexpr void reset() noexcept {
            vars_.fill({});
            flags_.reset();
        }
    };
    constexpr std::size_t usage_parse_msg_max_size = 128;
    template <auto& Config>
    class usage_parse_error {
    public:
        using config_type = std::remove_cvref_t<decltype(Config)>;
        using char_type = config_type::super_type::char_type;
        using string_type = std::basic_string_view<char_type>;
        static constexpr bool ref_enabled = std::is_same_v<char_type, char>;
    private:
        static constexpr std::string_view intro = "Parse error.\n \032 | \032\n \032 | \032\n\032";
        static constexpr std::string_view ref_fallback = "Preview is not available if char_type is not char.";
        static constexpr std::size_t ref_size = []() -> std::size_t {
            if constexpr (ref_enabled) {
                return ranges::max(Config.usages | views::transform([](const auto& usage) {
                    return usage.format.size();
                }));
            } else {
                return ref_fallback.size();
            }
        }();
        static constexpr std::size_t num_max_size = sizeof(std::size_t) * CHAR_BIT / 3; // overestimation
    public:
        static constexpr std::size_t what_size =
            intro.size() + (num_max_size + ref_size) * 2 + usage_parse_msg_max_size; // +'\0'
    private:
        std::array<char, what_size> _what{}; // null-terminated
    public:
        constexpr usage_parse_error(std::string_view msg, std::size_t usage_idx, std::size_t loc) noexcept {
            auto sec_start = intro.begin(), sec_end = sec_start - 1;
            char* current = _what.data();
            auto copy_sec = [&sec_start, &sec_end, &current]() {
                sec_start = sec_end + 1;
                sec_end = ranges::find(sec_start, intro.end(), '\032');
                current = ranges::copy(sec_start, sec_end, current).out;
            };
            copy_sec();
            char* after_num = std::to_chars(current, &*_what.end(), usage_idx + 1).ptr;
            std::size_t num_size = after_num - current;
            current = after_num;
            copy_sec();
            if constexpr (ref_enabled) {
                current = ranges::copy(Config.usages[usage_idx].format, current).out;
            } else {
                current = ranges::copy(ref_fallback, current).out;
            }
            copy_sec();
            current = ranges::fill(current, current + num_size, ' ');
            copy_sec();
            current = ranges::fill(current, current + loc, ' ');
            *(current++) = '^';
            copy_sec();
            ranges::copy(msg, current);
        }
        constexpr auto what() const noexcept {
            return std::string_view{_what.data()};
        }
    };
#define chk_err(msg) static_assert(sizeof(msg) - 1 <= usage_parse_msg_max_size, "Error message is too long!")
    template <const auto& Config, typename CharT, typename Hash, std::size_t FlagSetSize>
    constexpr auto _parse_usage(auto out) noexcept ->
    std::expected<parser_def<std::remove_cvref_t<decltype(Config)>>, usage_parse_error<Config>> {
#if __cpp_static_assert >= 202306L
#define rraise(msg, ...)\
    return std::unexpected{usage_parse_error<Config>{\
        msg, static_cast<size_t>(i), static_cast<size_t>(t.data() - usage.format.data() __VA_OPT__(+) __VA_ARGS__)\
    }}
#define raise(msg, ...)\
    chk_err(msg);\
    rraise(msg __VA_OPT__(,) __VA_ARGS__)
#else
#define rraise throw
#define raise throw
#endif
        using string_type = std::basic_string_view<CharT>;
        using enum parse_node_type;
        std::vector<parse_node<CharT>> tree;
        std::vector<string_type> var_names{{}};
        std::array<flag_info<ranges::size(Config.usages), CharT>, FlagSetSize> flag_set;
        for (auto [i, usage] : Config.usages | views::enumerate) {
            auto add_option = [&tree, &i, &usage] [[nodiscard]] (string_type t) ->
            std::optional<std::unexpected<usage_parse_error<Config>>> {
                bool compound = t.starts_with(Config.specials.compound_open);
                if (compound) {
                    if (t.ends_with(Config.specials.compound_close)) [[likely]] {
                        t.remove_prefix(1); t.remove_suffix(1);
                    } else {
                        raise("Unmatched '(' when declaring a compound option.");
                    }
                }
                std::size_t start_node_index = tree.size();
                auto rng = t | views::split(Config.specials.compound_divider);
                auto it = rng.begin();
                if (!compound && (ranges::advance(it, 2, rng.end()) == 0)) [[unlikely]] {
                    raise(
                        "Using | in a non-compound option declaration. "
                        "If you want to declare a compound option, enclose it with ().",
                        t.find('|')
                    );
                }
                for (const auto& opt : rng) {
                    if (opt.empty()) [[unlikely]] {
                        raise(
                            "Options cannot be empty. Check if you have added a redundant delimiter ('|').",
                            opt.data() - t.data()
                        );
                    }
                    tree.push_back({.type = option, .option_name = string_type{opt}});
                    tree.back().next_placeholder = tree.size();
                }
                tree.back().next_placeholder = 0;
                for (;start_node_index < tree.size(); ++start_node_index) {
                    tree[start_node_index].next = tree.size();
                }
                return std::nullopt;
            };
            auto add_var = [&var_names] (string_type name) -> std::size_t {
                auto search = ranges::find(var_names, name);
                if (search == var_names.end()) {
                    var_names.push_back(name);
                    return var_names.size() - 1;
                } else {
                    return search - var_names.begin();
                }
            };
            bool searching = !tree.empty();
            bool ended = false;
            std::size_t current = 0, prev; // current: final value is first unmatched node
            string_type t = usage.format;
            std::bitset<FlagSetSize> usage_flag_set;
            for (const auto& token : usage.format | views::split(Config.specials.delimiter)) {
                t = string_type{token};
                if (t.empty()) [[unlikely]] {
                    raise("Options cannot be empty. Check if you have added a redundant delimiter (' ').");
                }
                if (t.starts_with(Config.specials.flag_open)) {
                    if (t.ends_with(Config.specials.flag_close)) [[likely]] {
                        t.remove_prefix(1); t.remove_suffix(1);
                        if (!t.starts_with('-')) [[unlikely]] {
                            raise("Flags must be enclosed with a single pair of '[' and ']' and start with '-'.");
                        }
                        std::size_t eq_pos = t.find(Config.specials.equal);
                        string_type flag_name = t.substr(0, eq_pos);
                        std::size_t h = get_hash<Hash, FlagSetSize>(flag_name);
                        auto& flag = flag_set[h];
                        if (flag.defined()) {
                            if (flag.name != flag_name) [[unlikely]] {
                                raise("Hash collision when declaring flag.");
                            }
                            if (usage_flag_set[h]) [[unlikely]] {
                                raise("Re-declaring flag.");
                            }
                        } else {
                            flag.name = flag_name;
                        }
                        flag.defined_for[i] = true;
                        usage_flag_set[h] = true;
                        if (eq_pos != t.npos) {
                            string_type var_name = t.substr(eq_pos + 1);
                            if (
                                var_name.starts_with(Config.specials.var_open) &&
                                var_name.ends_with(Config.specials.var_close)
                            ) [[likely]] {
                                var_name.remove_prefix(1); var_name.remove_suffix(1);
                                flag.var_index_for[i] = add_var(var_name);
                            } else {
                                raise("A variable declaration must be enclosed with a pair of '<' and '>'.");
                            }
                        }
                    } else {
                        raise("Unclosed '[' when declaring flag.");
                    }
                    ended = true;
                    continue;
                }
                if (ended) [[unlikely]] {
                    raise("Declaring non-flag attributes after declaring the first flag.");
                }
                if (t.starts_with(Config.specials.var_open)) {
                    if (t.ends_with(Config.specials.var_close)) [[likely]] {
                        t.remove_prefix(1); t.remove_suffix(1);
                        parse_node<CharT> new_node{
                            .type = variable,
                            .var_index = add_var(t),
                            .next = tree.size() + 1
                        };
                        if (!tree.empty() && searching) {
                            while (tree[current].type == option && tree[current].next_placeholder) {
                                current = tree[current].next_placeholder;
                            }
                            switch (tree[current].type) {
                                case option:
                                    tree[current].next_placeholder = tree.size();
                                    break;
                                case variable:
                                    raise("Attempt to declare two variables on the same position.");
                                case end:
                                    tree[prev].next = tree.size();
                                    new_node.next = current;
                                    current = tree.size();
                            }
                        }
                        tree.push_back(new_node);
                        searching = false;
                    } else {
                        raise("Unclosed < when declaring variable.");
                    }
                } else {
                    if (!tree.empty() && searching) {
                        while (true) {
                            switch (tree[current].type) {
                                case option:
                                    if (tree[current].option_name == t) {
                                        prev = current;
                                        current = tree[current].next;
                                    } else if (tree[current].next_placeholder) {
                                        current = tree[current].next_placeholder;
                                        continue;
                                    } else {
                                        tree[current].next_placeholder = tree.size();
                                        if (auto res = add_option(t)) return *res;
                                        searching = false;
                                    }
                                    break;
                                case variable:
                                    raise("Declaring new option after a variable has been declared at the position.");
                                case end:
                                    tree[prev].next = tree.size();
                                    if (auto res = add_option(t)) return *res;
                                    tree.back().next_placeholder = current;
                                    tree[current].next = tree.size();
                                    searching = false;
                                    break;
                            }
                            break;
                        }
                    } else {
                        if (auto res = add_option(t)) return *res;
                    }
                }
            }
            if (!tree.empty() && tree[current].type == end) [[unlikely]] {
                raise("Duplicate usage.");
            }
            if (searching) {
                while (tree[current].next_placeholder) {
                    current = tree[current].next_placeholder;
                }
                tree[current].next_placeholder = tree.size();
            }
            tree.push_back({.type = end, .usage_index = static_cast<size_t>(i)});
#undef rraise
#undef raise
        }
        if constexpr (!std::is_same_v<decltype(out), std::nullptr_t>) {
            ranges::copy(Config.usages, out->usages.begin());
            ranges::copy(tree, out->tree.begin());
            ranges::copy(var_names, out->var_names.begin());
            ranges::copy(flag_set, out->flag_set.begin());
        }
        return parser_def{
            .usage_size = ranges::size(Config.usages),
            .tree_size = tree.size(),
            .vars_size = var_names.size(),
            .flag_set_size = FlagSetSize,
            .var_num = var_names.size(),
            .config = Config
        };
    }
#undef chk_err
    template <config_instance auto& Config>
    using config_type_of = std::remove_cvref_t<decltype(Config)>::super_type;
    template <
        config_instance auto& Config,
        std::size_t FlagSetSize = 512,
        typename Hash = hash<std::basic_string_view<typename config_type_of<Config>::char_type>>
    >
    requires requires(std::basic_string_view<typename config_type_of<Config>::char_type> str) {
        {Hash{}(str)} -> std::same_as<size_t>;
    }
    consteval auto define_parser() noexcept {
        using config_type = config_type_of<Config>;
        using result_type = config_type::result_type;
        using char_type = config_type::char_type;
        constexpr auto res = _parse_usage<Config, char_type, Hash, FlagSetSize>(nullptr);
        if constexpr (res) {
            parser_info<result_type, char_type, Hash, *res> info;
            _parse_usage<Config, char_type, Hash, FlagSetSize>(&info);
            return info;
        } else {
#if __cpp_static_assert >= 202306L
            static_assert(false, res.error().what());
#endif
        }
    }
}

template <cmd::tagged<cmd::usage_range_tag> Rng>
struct std::formatter<Rng, typename Rng::format_char_type> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }
    constexpr auto format(const Rng& rng, auto& ctx) const {
        auto out = ctx.out();
        for (const auto& usage : rng.value) {
            out = std::format_to(out, Rng::format, usage.format);
        }
        return out;
    }
};

template <cmd::tagged<cmd::error_tag> Error>
struct std::formatter<Error, typename Error::super_type::format_char_type> {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }
    constexpr auto format(const Error& error, auto& ctx) const {
        return error.print(ctx.out());
    }
};

template <cmd::tagged<cmd::error_ref_tag> Ref>
struct std::formatter<Ref, typename Ref::super_type::format_char_type> {
    using parser_type = Ref::super_type;
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }
    constexpr auto format(const Ref& ref, auto& ctx) const {
        constexpr const auto& config = parser_type::config;
        if (ref.loc.arg_loc == -1uz) {
            return ctx.out();
        } else {
            auto out = std::format_to(ctx.out(), config.ref_tmpl, ref.template wrap<0>());
            return std::format_to(out, config.ref_tmpl, ref.template wrap<1>());
        }
    }
};

template <cmd::tagged<cmd::error_ref_c_tag> RefC>
struct std::formatter<RefC, typename RefC::super_type::format_char_type> {
    using parser_type = RefC::super_type;
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }
    constexpr auto format(const RefC& ref_c, auto& ctx) const {
        auto out = ctx.out();
        const auto& ref = ref_c.ref;
        constexpr const auto& specials = parser_type::config.specials;
        bool first = true, indicated = false;
        std::size_t i = 0;
        for (auto arg : ref.args) {
            if (first) {
                first = false;
            } else {
                *(out++) = specials.delimiter;
            }
            if constexpr (RefC::mode == 0) {
                out = ranges::copy(arg, out).out;
            } else if constexpr (RefC::mode == 1) {
                for (std::size_t j = 0; j < arg.size(); ++j) {
                    if ((i == ref.loc.arg_loc) && (j == ref.loc.in_arg_loc)) [[unlikely]] {
                        *(out++) = specials.indicator;
                        indicated = true;
                    } else {
                        *(out++) = specials.delimiter;
                    }
                }
            }
            ++i;
        }
        if ((RefC::mode == 1) && !indicated) {
            *(out++) = specials.indicator;
        }
        return out;
    }
};