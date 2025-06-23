#pragma once
#include <array>
#include <algorithm>
#include <string>
#include <bitset>
#include <vector>
#include <expected>
#include <iostream>
#include <utility>
#include "common.hpp"
#include "chartypes.hpp"
#include "hash.hpp"
#include "error.hpp"

namespace cmd {
    /// All types of error that can be raised during parsing.\n
    /// Guarantees:
    /// 1. The underlying values start from 0, and are consecutive.
    /// 2. unknown_error is the last error_type.
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

    /// A concept that checks if T can be used to identify usages.
    /// Uniqueness is not required.
    template <typename T>
    concept usage_id = std::is_integral_v<T> || std::is_enum_v<T>;
    struct usage_tag;
    /// A usage defines a way to use the CLI.
    template <usage_id Result, char_like CharT>
    struct usage {
        using tag = usage_tag;
        using result_type = Result;
        using char_type = CharT;
        using string_view_type = std::basic_string_view<char_type>;
        /// Expressed in the grammar specified in README.
        string_view_type format;
        /// Used to identify usage.
        Result name;
    };

    /// Implementation details. Use at your own risk.
    namespace detail {
        struct usage_range_tag;
        template <auto& FormatString>
        requires (
            is_template_instance_v<std::basic_string_view, std::remove_cvref_t<decltype(FormatString)>> &&
            char_like<typename std::remove_cvref_t<decltype(FormatString)>::value_type>
        )
        struct usage_range {
            template <typename Rng>
            requires (
                ranges::input_range<Rng> &&
                tagged<std::iter_value_t<ranges::range_value_t<Rng>>, usage_tag>)
            struct type {
                using tag = usage_range_tag;
                using char_type = std::iter_value_t<ranges::range_value_t<Rng>>::char_type;
                using format_string_view_type = std::remove_cvref_t<decltype(FormatString)>;
                using format_char_type = format_string_view_type::value_type;
                static constexpr const format_string_view_type& format = FormatString;
                const Rng& value;
            };
        };
    }

    /// Defines special characters in parsing.
    /// \tparam CharT: input character type
    template <char_like CharT>
    struct special_chars {
        CharT delimiter, enter, quote_open, quote_close, escape, indicator;
        std::basic_string_view<CharT> compound_open , compound_close, compound_divider,
            flag_open, flag_close, flag_prefix,
            var_open, var_close, var_capture, equal, variadic;
    };
    struct config_tag;
    /// Default values for `man_tmpl`, `error_tmpl`, `ref_tmpl`,
    /// `specials` and `error_msgs` in `config`.
    template <char_like CharT, char_like FmtCharT>
    struct config_default;
    /// Configuration of a parser.
    /// \tparam Result: type for identifying usages
    /// \tparam CharT: input character type
    /// \tparam FmtCharT: output character type
    /// \tparam CharTraits: `CharTraits` for `CharT`
    /// \tparam FmtCharTraits: `CharTraits` for `FmtCharT`
    template <usage_id Result, char_like CharT = char, char_like FmtCharT = CharT,
        typename CharTraits = std::char_traits<CharT>, typename FmtCharTraits = std::char_traits<FmtCharT>>
    struct config {
        using tag = config_tag;
        using result_type = Result;
        using char_type = CharT;
        using format_char_type = FmtCharT;
        using char_traits_type = CharTraits;
        using format_char_traits_type = FmtCharTraits;
        using string_view_type = std::basic_string_view<char_type, char_traits_type>;
        using format_string_view_type = std::basic_string_view<format_char_type, format_char_traits_type>;
        using usage_type = usage<result_type, char_type>;
        using config_default_type = config_default<char_type, format_char_type>;
        template <size_t N>
        struct type {
            using super_type = config;
            format_string_view_type name, description;
            usage_type usages[N];
            format_string_view_type explanation;
            /// `std::basic_format_string<FmtCharT>` that accepts three arguments (in order).
            /// \param 0: name of the program
            /// \param 1: description of the program
            /// \param 2: list of usages
            /// \param 3: detailed explanation of usages
            format_string_view_type man_tmpl = config_default_type::man_tmpl;
            /// `std::basic_format_string<FmtCharT>` that accepts three arguments (in order).
            /// \param 0: error message from `error_msgs`
            /// \param 1: location of error (in command)
            /// \param 2: list of closest usages
            /// \param 3: extra information (to error message)
            format_string_view_type error_tmpl = config_default_type::error_tmpl;
            /// `std::basic_format_string<FmtCharT>` that accepts one argument.
            /// \param 0: usage string
            format_string_view_type usage_tmpl = config_default_type::usage_tmpl;
            /// Special characters in command parsing.
            special_chars<char_type> specials = config_default_type::specials;
            /// Order corresponds to order of enums in `error_type`.
            std::array<format_string_view_type, error_types_n> error_msgs = config_default_type::error_msgs;
        };
    };
    template <typename T>
    concept config_instance = tagged<typename T::super_type, config_tag>;

    enum class parse_node_type {
        option, variable, variable_option, variadic, end
    };
    /// A node in a parse tree.
    /// \tparam CharT: input character type
    template <char_like CharT>
    struct parse_node {
        parse_node_type type;
        /// Only applicable when `type` is `option` or `variable_option`.
        std::basic_string_view<CharT> option_name{};
        /// Made union for clearer semantics.
        union {
            /// Only applicable when `type` is `variable` or `variable_option`.
            std::size_t var_index = 0;
            /// Only applicable when `type` is `end`.
            std::size_t usage_index;
        };
        /// Index of the next node in the same position. A value of 0 means there is no such node.
        std::size_t next_placeholder = 0;
        /// Index of the first node in the next position. A value of 0 means there is no such node.
        std::size_t next = 0;
    };

    /// Information of a flag.
    template <size_t UsageSize, char_like CharT>
    struct flag_info {
        std::basic_string_view<CharT> name;
        std::bitset<UsageSize> defined_for;
        /// Index of the capturing variable for the flag in each usage.
        /// A value of 0 means it does not exist.
        std::array<size_t, UsageSize> var_index_for{};
        constexpr bool defined() const noexcept {
            return defined_for.any();
        }
    };

    /// A concept that checks if `Hash` is a "hasher" for `CharT`.
    /// \attention Semantic requirement: the hash function must be constexpr.
    template <typename Hash, typename CharT>
    concept hasher = requires(std::basic_string_view<CharT> str) {
        {Hash{}(str)} -> std::same_as<std::size_t>;
    };
    /// \return hash of str (by hasher `Hash`) in a set of size `SetSize`
    template <typename Hash, std::size_t SetSize>
    static constexpr std::size_t get_hash(auto str) noexcept {
        return Hash{}(str) % SetSize;
    }

    struct parser_def_tag;
    /// Definition of a parser.
    /// \tparam Config: configuration of parser
    template <config_instance Config>
    struct parser_def {
        using tag = parser_def_tag;
        using config_type = Config::super_type;
        std::size_t usage_size, tree_size, vars_size, flag_set_size, var_num;
        const Config& config;
    };

    struct parser_info_tag;
    /// (Computed) information of parser.
    /// \tparam Def: parser definition
    /// \tparam Hash: hasher
    template <tagged<parser_def_tag> auto Def, hasher<typename decltype(Def)::config_type::char_type> Hash>
    struct parser_info {
        using tag = parser_info_tag;
        using config_type = decltype(Def)::config_type;
        using result_type = config_type::result_type;
        using char_type = config_type::char_type;
        using hash_type = Hash;
        using string_view_type = config_type::string_view_type;
        static constexpr auto def = Def;
        std::array<usage<result_type, char_type>, Def.usage_size> usages;
        std::array<parse_node<char_type>, Def.tree_size> tree;
        std::array<string_view_type, Def.vars_size> var_names;
        std::array<flag_info<Def.usage_size, char_type>, Def.flag_set_size> flag_set;
    };

    /// Location of an error in a command.
    struct error_loc {
        /// Index of argument. -1: error cannot be pinpointed.
        std::size_t arg_loc = -1;
        /// Index of character in the argument. -1: subject is invalid
        /// (e.g. when it is thrown as part of an `parser::argument_error`, it means the variable does not exist).
        std::size_t in_arg_loc = -1;
    };
    /// A binding to `args`.
    /// It borrows if `Args` is an l-value reference, or takes ownership otherwise.
    /// \remark `receiver` is not copyable nor movable when it borrows from `args`.
    /// This is to prevent UB from lifetime issues.
    template <typename Args>
    struct receiver {
        std::remove_reference_t<Args> args;
    };
    template <typename Args>
    struct receiver<Args&> {
    protected:
        constexpr receiver(const receiver&) = default;
        constexpr receiver(receiver&&) noexcept = default;
        constexpr receiver& operator=(const receiver&) = default;
        constexpr receiver& operator=(receiver&&) noexcept = default;
    public:
        Args& args;
        constexpr receiver(Args& args) noexcept : args{args} {}
    };
    struct error_ref_c_tag;
    struct error_ref_tag;
    struct error_tag;
    struct parse_result_tag;
    template <tagged<parser_info_tag> auto& Info>
    class parser {
    public:
        static constexpr const auto& info = Info;
        static constexpr const auto& def = Info.def;
        static constexpr const auto& config = Info.def.config;
        using info_type = std::remove_cvref_t<decltype(Info)>;
        using def_type = std::remove_cvref_t<decltype(def)>;
        using config_type = info_type::config_type;
        using result_type = config_type::result_type;
        using char_type = config_type::char_type;
        using char_traits_type = config_type::char_traits_type;
        using string_type = std::basic_string<char_type, char_traits_type>;
        using string_view_type = config_type::string_view_type;
        using format_char_type = config_type::format_char_type;
        using format_char_traits_type = config_type::format_char_traits_type;
        using format_string_type = std::basic_string<format_char_type, format_char_traits_type>;
        using format_string_view_type = config_type::format_string_view_type;
        using hash_type = info_type::hash_type;
        using refs_type = std::vector<const typename config_type::usage_type*>;
        using usage_range_type = detail::usage_range<config.usage_tmpl>;
        static constexpr auto input_stream = input_object<char_type>::value;
        static constexpr auto output_stream = output_object<format_char_type>::value;
        static constexpr bool inputtable{input_stream};
        static constexpr bool outputtable = std::formattable<format_string_type, format_char_type>;
        /// Despite its name, the struct only stores the `index` of a variable.
        /// This is because the `index` is computed at compile time from variable name.
        struct var_name {
            std::size_t index;
            consteval var_name(const char_type* name) {
                for (auto [i, var_name] : Info.var_names | views::enumerate) {
                    if (name == var_name) {
                        index = i;
                        return;
                    }
                }
                throw std::invalid_argument("Unknown variable");
            }
        };
        /// Despite its name, the struct only stores the `index` of a flag.
        /// This is because the `index` is computed at compile time from flag name.
        struct flag_name {
            std::size_t index;
            consteval flag_name(const char_type* name) {
                const std::size_t h = get_hash<hash_type, Info.flag_set.size()>(name);
                if (name == Info.flag_set[h].name) [[likely]] {
                    index = h;
                } else {
                    throw std::invalid_argument("Unknown flag. Note that preceding '-'(s) must be included.");
                }
            }
        };
    private:
        struct vars_element {
            string_type content{};
            error_loc loc{};
        };
        /// \internal Index 0: variadic argument.\n Index 1 onwards: variables.
        std::array<vars_element, Info.var_names.size()> vars_{};
        std::bitset<Info.flag_set.size()> flags_{};
        std::vector<string_type> variadic_{};
    public:
        /// A wrapper for `error_ref` for `std::formatter`.
        /// \tparam Mode: 0 - prints the command (`ref`)\n
        /// 1 - prints an indicator (^) at the location of the error
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
        /// An error reference.
        template <typename Args>
        requires RANGE_OF(Args, forward_range, string_view_type)
        struct error_ref : receiver<Args> {
            using tag = error_ref_tag;
            using super_type = parser;
            error_loc loc;
            /// Binds to `args` and set error location to invalid.
            explicit constexpr error_ref(Args&& args) :
                receiver<Args>{std::forward<Args>(args)}, loc{-1uz} {}
            /// Binds to `args` and set error location to `loc`.
            constexpr error_ref(Args&& args, error_loc loc) :
                receiver<Args>{std::forward<Args>(args)}, loc{loc} {}

            template <int Mode>
            constexpr auto wrap() const noexcept {
                return typename error_ref_wrapper<Mode>::type{*this};
            }
        };
        /// An error emitted during `parser::parse`,
        /// without error location due to incompatible type of `args`.
        struct part_parse_error {
            using tag = error_tag;
            using super_type = parser;
            error_type type;
            refs_type refs;
        };
        /// An error emitted during `parser::parse`.
        template <typename Args>
        struct parse_error : part_parse_error {
            error_ref<Args> ref;
            /// Make a `parse_error` whose location cannot be pinpointed
            /// \param type: error type
            /// \param args: an `forward_range` of arguments that form the command
            /// \param refs: vector of pointers of related usages
            constexpr parse_error(error_type type, Args&& args, refs_type refs = {}) noexcept :
                part_parse_error{type, std::move(refs)}, ref{std::forward<Args>(args)} {}
            /// Make a `parse_error` that has a defined location
            /// \param loc: location of the error
            constexpr parse_error(error_type type, Args&& args, error_loc loc, refs_type refs = {}) noexcept :
                part_parse_error{type, std::move(refs)}, ref{std::forward<Args>(args), loc} {}
            constexpr parse_error(const parse_error&) = default;
            constexpr parse_error(parse_error&&) noexcept = default;

            auto print(std::output_iterator<format_char_type> auto out) const
            requires outputtable {
                return std::format_to(
                    out, config.error_tmpl, config.error_msgs[std::to_underlying(this->type)],
                    ref, typename usage_range_type::type{this->refs}, format_string_view_type{});
            }
            auto print() const
            requires (!!output_stream && outputtable) {
                return print(std::ostreambuf_iterator{*output_stream});
            }
        };
        /// An error emitted by the developer for invalid arguments.
        /// \remark `ref` always borrows,
        /// so `argument_error` is always non-copyable and non-movable.
        template <typename Args>
        struct argument_error {
            using tag = error_tag;
            using super_type = parser;
            static constexpr error_type type = error_type::invalid_argument;
            format_string_view_type what; /// Error message.
            error_ref<const Args&> ref;
            std::size_t usage_index;

            /// \param args: an `forward_range` of arguments that form the command
            /// \param loc: location of the error
            constexpr argument_error(
                decltype(what) what, const Args& args, error_loc loc, std::size_t usage_index) noexcept :
                what{what}, ref{args, loc}, usage_index{usage_index} {}

            auto print(std::output_iterator<format_char_type> auto out) const
            requires outputtable {
                typename usage_range_type::type refs{std::array{&Info.usages[usage_index]}};
                return std::format_to(
                    out, config.error_tmpl, config.error_msgs[std::to_underlying(type)], ref, refs, what);
            }
            auto print() const
            requires (!!output_stream && outputtable) {
                return print(std::ostreambuf_iterator{*output_stream});
            }
        };
        /// Result of a successful `parse`,
        /// without error location due to incompatible type of `args`.
        struct part_parse_result {
            using tag = parse_result_tag;
            using super_type = parser;
            result_type result;
            std::size_t usage_index;
        };
        /// Result of a successful `parse`.
        template <typename Args>
        requires RANGE_OF(Args, forward_range, string_view_type)
        struct parse_result : part_parse_result, receiver<Args> {
            constexpr parse_result(result_type result, std::size_t usage_index, Args&& args) :
                part_parse_result{result, usage_index}, receiver<Args>{std::forward<Args>(args)} {}
        };
    protected:
        /// \param node_loc: index of the last node (in `Info.tree`)
        /// reached during `parse` before error
        /// \return: vector of related usages
        static constexpr refs_type search_refs(std::size_t node_loc) noexcept {
            using enum parse_node_type;
            std::array<std::size_t, Info.tree.size()> deque;
            std::array<bool, Info.tree.size()> visited{};
            refs_type refs;
            refs.reserve(Info.usages.size());
            auto front = deque.begin(), back = front + 1;
            *front = node_loc;
            auto push = [&back, &visited](std::size_t node_idx) {
                if (!visited[node_idx]) {
                    *(back++) = node_idx;
                    visited[node_idx] = true;
                }
            };
            while (front != back) {
                const auto& node = Info.tree[*front];
                switch (node.type) {
                    case option:
                    case variable_option: {
                        push(node.next);
                        std::size_t next_option = node.next_placeholder;
                        if (next_option) {
                            *front = next_option;
                            continue;
                        }
                        break;
                    }
                    case variable:
                        push(node.next);
                        break;
                    case end:
                        refs.push_back(Info.usages.data() + node.usage_index);
                        break;
                }
                ++front;
            }
            return refs;
        }
    public:
        /// \param args: an `input_range` of arguments that form a command
        /// \return If `Args` is a forward range,
        /// return an `std::expected<parse_result, parse_error>`;
        /// otherwise, return an `std::expected<part_parse_result, part_parse_error>`.
        template <typename Args>
        requires RANGE_OF(Args, input_range, string_view_type)
        constexpr auto parse(Args&& args) noexcept {
            using enum parse_node_type;
            using enum error_type;
            using args_type = std::remove_cvref_t<Args>;
            using iter_type = ranges::iterator_t<args_type>;
            using sentinel_type = ranges::sentinel_t<args_type>;
            constexpr bool full_result = ranges::forward_range<Args>;
            using return_type = std::conditional_t<full_result,
                std::expected<parse_result<Args>, parse_error<Args>>,
                std::expected<part_parse_result, part_parse_error>>;
            auto node = Info.tree.begin();
            auto start_node = node;
            auto arg_current = ranges::begin(args);
            auto arg_end = ranges::end(args);
            std::size_t arg_loc = 0;
            auto next_arg = [&arg_current, &arg_loc]() {
                ++arg_current; ++arg_loc;
            };
            auto next_arg_node = [&start_node, &node, next_arg](std::size_t next_node_idx) {
                start_node = node = Info.tree.begin() + next_node_idx;
                next_arg();
            };
            auto raise = [&start_node, &args, &arg_loc](error_type err, std::size_t in_arg_loc = 0) {
                refs_type refs = search_refs(start_node - Info.tree.begin());
                if constexpr (full_result) {
                    return return_type{std::unexpect,
                        err, std::forward<Args>(args), error_loc{arg_loc, in_arg_loc}, refs};
                } else {
                    return return_type{std::unexpect, err, refs};
                }
            };
            string_view_type arg;
            while (true) {
                if (arg_current == arg_end) [[unlikely]] {
                    while (node->next_placeholder) {
                        node = Info.tree.begin() + node->next_placeholder;
                    }
                    if (node->type != variadic && node->type != end) {
                        return raise(too_few_arguments);
                    }
                } else {
                    arg = *arg_current;
                }
                parse_arg:
                switch (node->type) {
                    case option:
                    case variable_option: {
                        if (node->option_name == arg) {
                            if (node->type == variable_option) {
                                vars_[node->var_index] = {string_type{arg}, {arg_loc, 0}};
                            }
                            next_arg_node(node->next);
                        } else if (node->next_placeholder) {
                            node = Info.tree.begin() + node->next_placeholder;
                            if !consteval {goto parse_arg;}
                        } else [[unlikely]] {
                            return raise(unknown_option);
                        }
                        continue;
                    }
                    case variable: {
                        if (arg.starts_with(config.specials.flag_prefix)) {
                            return raise(flag_cannot_be_variable);
                        }
                        vars_[node->var_index] = {string_type{arg}, {arg_loc, 0}};
                        next_arg_node(node->next);
                        continue;
                    }
                    case variadic: {
                        if constexpr (std::sized_sentinel_for<iter_type, sentinel_type>) {
                            variadic_.reserve(ranges::end(args) - arg_current);
                        } else if constexpr (ranges::sized_range<args_type>) {
                            variadic_.reserve(ranges::size(args));
                        }
                        for (; arg_current != arg_end; next_arg()) {
                            arg = *arg_current;
                            if (arg.starts_with(config.specials.flag_prefix)) break;
                            variadic_.emplace_back(arg);
                        }
                        break;
                    }
                    case end: {
                        break;
                    }
                }
                break;
            }
            for (;arg_current != arg_end; ++arg_current) {
                string_view_type flag_str = *arg_current;
                if (flag_str.starts_with(config.specials.flag_prefix)) [[likely]] {
                    const std::size_t eq_pos = flag_str.find(config.specials.equal);
                    string_view_type flag_name = flag_str.substr(0, eq_pos);
                    const std::size_t h = get_hash<hash_type, Info.flag_set.size()>(flag_name);
                    const auto& flag = Info.flag_set[h];
                    if (flag.name == flag_name && flag.defined_for[node->usage_index]) [[likely]] {
                        flags_[h] = true;
                        if (eq_pos != flag_str.npos) {
                            if (std::size_t var_index = flag.var_index_for[node->usage_index]) [[likely]] {
                                vars_[var_index] = {
                                    string_type{flag_str.substr(eq_pos + config.specials.equal.size())},
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
            if constexpr (full_result) {
                return return_type{std::in_place,
                    Info.usages[node->usage_index].name, node->usage_index, std::forward<Args>(args)};
            } else {
                return return_type{std::in_place, Info.usages[node->usage_index].name, node->usage_index};
            }
        }
        /// \param argc: number of arguments
        /// \param argv: array of arguments
        /// \return `std::expected<parse_result, parse_error>`
        /// \remark This method is meant to be called with `argc` and `argv`
        /// from the `main` function.
        /// As such, it expects the first argument in `argv` to be the path of the program,
        /// and therefore discarded.
        constexpr auto parse(int argc, char* argv[]) noexcept
        requires (std::same_as<char_type, char>) {
            return parse(views::counted(argv + 1, argc - 1));
        }
        /// When `char_type` != `char`, `parse` does a conversion from `char_type` to `char`
        /// using `std::codecvt` with the system locale.
        constexpr auto parse(int argc, char* argv[])
        requires (!std::same_as<char_type, char>) {
            std::vector<string_type> args;
            args.reserve(argc - 1);
            const translator<char, char_type, true> t{};
            for (std::string_view arg : views::counted(argv + 1, argc - 1)) {
                args.push_back(t(arg, t.default_size_mul, ""));
            }
            return parse(std::move(args));
        }
        /// \param str: an `input_range` of `char_type` that forms a command string
        /// \return `std::expected<parse_result, parse_error>`
        /// \remark This method creates a vector of arguments that is parsed from `str`.
        template <typename Str>
        requires RANGE_OF(Str, input_range, char_type)
        constexpr auto parse(const Str& str) {
            using enum error_type;
            std::vector<string_type> args;
            args.assign({{}});
            bool quote_open = false, escape = false;
            for (const char_type& c : str) {
                if (quote_open || !char_traits_type::eq(c, config.specials.delimiter)) {
                    if (escape) {
                        escape = false;
                    } else {
                        if constexpr (char_traits_type::eq(config.specials.quote_open, config.specials.quote_close)) {
                            if (char_traits_type::eq(c, config.specials.quote_open)) {
                                quote_open = !quote_open;
                                continue;
                            }
                        } else {
                            if (char_traits_type::eq(c, config.specials.quote_open)) {
                                quote_open = true;
                                continue;
                            }
                            if (char_traits_type::eq(c, config.specials.quote_close)) {
                                quote_open = false;
                                continue;
                            }
                        }
                        if (char_traits_type::eq(c, config.specials.escape)) {
                            escape = true;
                            continue;
                        }
                    }
                    args.back().push_back(c);
                } else {
                    if (!args.back().empty()) args.push_back({});
                }
            }
            if (args.front().empty()) args.clear();

            auto get_return = [this, &args]() {return parse(std::move(args));};
            using return_type = std::invoke_result_t<decltype(get_return)>;
            if (quote_open || escape) [[unlikely]] {
                return return_type{std::unexpect, open_special_character, std::move(args)};
            }
            return get_return();
        }
        /// Reads and parses a line from standard input.
        /// \return `std::expected<parse_result, parse_error>`
        auto readline()
        requires inputtable {
            using iter_type = std::istreambuf_iterator<char_type>;
            *input_stream >> std::ws;
            return parse(
                ranges::subrange(iter_type(*input_stream), iter_type())
                | views::take_while([](char_type c) {
                    const char_type eol = input_stream->widen('\n');
                    if (char_traits_type::eq(c, eol)) {
                        input_stream->ignore(std::numeric_limits<std::streamsize>::max(), eol);
                        return false;
                    } else {
                        return true;
                    }
                }));
        }
        auto readline()
        requires (!inputtable) {
            std::string command;
            std::getline(std::cin, command);
            return parse(translator<char, char_type, true>{}(command));
        }
        /// \param result: `parse_result` from `parse`
        /// \param name: name of the variable that causes the error
        /// \param what: error message
        template <typename Rng>
        constexpr argument_error<std::remove_reference_t<Rng>> raise_argument_error(
            const parse_result<Rng>& result, var_name name, format_string_view_type what) const noexcept {
            return {what, result.args, vars_[name.index].loc, result.usage_index};
        }
        /// Prints program manual to an `output_iterator` `out`.
        /// \param args: extra arguments for `config.man_tmpl`
        void print_man(std::output_iterator<format_char_type> auto out, const auto&... args) const
        requires outputtable {
            std::format_to(
                out, config.man_tmpl, config.name, config.description,
                typename usage_range_type::type{views::iota(Info.usages.begin(), Info.usages.end())},
                config.explanation, args...);
        }
        /// Prints program manual to standard output.
        void print_man(const auto&... args) const
        requires (!!output_stream && outputtable) {
            print_man(std::ostreambuf_iterator{*output_stream}, args...);
        }
        /// \return value of variable named `name`
        constexpr auto&& var(this auto&& self, var_name name) noexcept {
            return self.vars_[name.index].content;
        }
        /// \return whether flag named `name` is set
        /// \remark The prefix of a flag has to be included in `name`.
        constexpr bool flag(flag_name name) const noexcept {
            return flags_[name.index];
        }
        /// \return vector of variadic variables captured during `parse`
        constexpr const std::vector<string_type>& variadic() const noexcept {
            return variadic_;
        }
        /// Clears values of all variables and sets all flags to `false`.
        constexpr void reset() noexcept {
            vars_.fill({});
            flags_.reset();
            variadic_.clear();
        }
    };

    namespace detail {
        template <const auto& Config, typename CharT, typename Hash, std::size_t FlagSetSize>
        constexpr auto parse_usage(auto out) noexcept ->
        std::expected<parser_def<std::remove_cvref_t<decltype(Config)>>, define_error> {
            using string_view_type = std::basic_string_view<CharT>;
            using enum parse_node_type;
            using enum define_error_ref_type;
            std::vector<parse_node<CharT>> tree;
            std::vector<string_view_type> var_names{{}};
            std::array<flag_info<ranges::size(Config.usages), CharT>, FlagSetSize> flag_set;
            auto add_var = [&var_names] (string_view_type name) -> std::size_t {
                auto search = ranges::find(var_names, name);
                if (search == var_names.end()) {
                    var_names.push_back(name);
                    return var_names.size() - 1;
                } else {
                    return search - var_names.begin();
                }
            };
            for (auto [i, usage] : Config.usages | views::enumerate) {
                bool searching = !tree.empty();
                bool ended = false;
                parse_node_type end_type = end;
                // current node in traversal, final value is first unmatched node
                std::size_t current = 0;
                string_view_type t = usage.format;
                std::bitset<FlagSetSize> usage_flag_set;
                auto get_error = [&i, &t, &usage](std::string_view what, std::size_t offset = 0) -> define_error_ref {
                    return {what, i, t.data() - usage.format.data() + offset};
                };
                auto raise = [get_error](std::string_view what, std::size_t offset = 0) {
                    return std::unexpected(define_error{Config, std::array{get_error(what, offset)}});
                };
                /// \param exclude: needs to be sorted
                auto add_option =
                [&tree, &t, raise] [[nodiscard]]
                (parse_node_type type = option, std::size_t var_index = 0, std::span<string_view_type> exclude = {})
                -> std::optional<std::unexpected<define_error>> {
                    bool compound = t.starts_with(Config.specials.compound_open);
                    if (compound) {
                        if (t.ends_with(Config.specials.compound_close)) [[likely]] {
                            t.remove_prefix(Config.specials.compound_open.size());
                            t.remove_suffix(Config.specials.compound_close.size());
                        } else {
                            return raise("Unmatched '(' when declaring a compound option.");
                        }
                    }
                    std::size_t start_node_index = tree.size();
                    auto rng = t | views::split(Config.specials.compound_divider);
                    if (rng.empty()) [[unlikely]] {
                        return raise("Option lists cannot be empty.");
                    }
                    auto it = rng.begin();
                    if (compound) {
                        if (t.ends_with(Config.specials.var_capture)) {
                            t.remove_suffix(Config.specials.var_capture.size());
                            if (t.empty() || t.ends_with(Config.specials.compound_divider)) {
                                if (var_index) [[likely]] {
                                    tree.push_back({
                                        .type = variable,
                                        .var_index = var_index,
                                        .next = tree.size() + 1
                                    });
                                    return std::nullopt;
                                } else {
                                    return raise(
                                        R"(Using a capture ("...") without specifying the capturing variable.)",
                                        t.size());
                                }
                            }
                        }
                    } else {
                        if (ranges::advance(it, 2, rng.end()) == 0) [[unlikely]] {
                            return raise(
                                "Using | in a non-compound option declaration. "
                                "If you want to declare a compound option, enclose it with ().",
                                t.find(Config.specials.compound_divider));
                        }
                    }
                    for (const auto& opt : rng) {
                        string_view_type opt_t{opt};
                        if (opt_t.empty()) [[unlikely]] {
                            return raise(
                                "Options cannot be empty. Check if you have added a redundant delimiter ('|').",
                                opt.data() - t.data());
                        }
                        if (opt_t == Config.specials.var_capture) [[unlikely]] {
                            return raise(
                                R"(A capture ("...") must be placed as the last option.)", opt.data() - t.data());
                        }
                        if (ranges::binary_search(exclude, opt_t)) [[unlikely]] {
                            return raise(
                                "Variable option repeats an existing option in this position.", opt.data() - t.data());
                        }
                        tree.push_back({.type = type, .option_name = string_view_type{opt}, .var_index = var_index});
                        tree.back().next_placeholder = tree.size();
                    }
                    tree.back().next_placeholder = 0;
                    for (;start_node_index < tree.size(); ++start_node_index) {
                        tree[start_node_index].next = tree.size();
                    }
                    return std::nullopt;
                };
                // call after pushing
                auto insert_before_end = [&tree, &current](std::size_t added_first) {
                    tree.back().next_placeholder = added_first;
                    tree[current].next = tree.size();
                    std::swap(tree[current], tree[added_first]);
                };
                for (const auto& token : usage.format | views::split(Config.specials.delimiter)) {
                    t = string_view_type{token};
                    if (t.empty()) [[unlikely]] {
                        return raise("Options cannot be empty. Check if you have added a redundant delimiter (' ').");
                    }
                    if (t.starts_with(Config.specials.flag_open)) {
                        if (t.ends_with(Config.specials.flag_close)) [[likely]] {
                            t.remove_prefix(Config.specials.flag_open.size());
                            t.remove_suffix(Config.specials.flag_close.size());
                            if (!t.starts_with('-')) [[unlikely]] {
                                return raise(
                                    "Flags must be enclosed with a single pair of '[' and ']' and start with '-'.");
                            }
                            std::size_t eq_pos = t.find(Config.specials.equal);
                            string_view_type flag_name = t.substr(0, eq_pos);
                            std::size_t h = get_hash<Hash, FlagSetSize>(flag_name);
                            auto& flag = flag_set[h];
                            if (flag.defined()) {
                                std::string_view what;
                                if (flag.name != flag_name) [[unlikely]] {
                                    what = "Hash collision when declaring flag.";
                                } else if (usage_flag_set[h]) [[unlikely]] {
                                    what = "Re-declaring flag.";
                                }
                                if (what.data()) {
                                    std::array<define_error_ref, 2> refs{get_error(what), {
                                        .what = "Previous flag defined here.",
                                        .usage_index = 0,
                                        .type = note
                                    }};
                                    for (std::size_t& j = refs[1].usage_index; j < ranges::size(Config.usages); ++j) {
                                        if (flag.defined_for[j]) {
                                            refs[1].loc = flag.name.data() - Config.usages[j].format.data();
                                            break;
                                        }
                                    }
                                    return std::unexpected(define_error{Config, refs});
                                }
                            } else {
                                flag.name = flag_name;
                            }
                            flag.defined_for[i] = true;
                            usage_flag_set[h] = true;
                            if (eq_pos != t.npos) {
                                string_view_type var_name = t.substr(eq_pos + 1);
                                if (
                                    var_name.starts_with(Config.specials.var_open) &&
                                    var_name.ends_with(Config.specials.var_close)
                                ) [[likely]] {
                                    var_name.remove_prefix(1); var_name.remove_suffix(1);
                                    flag.var_index_for[i] = add_var(var_name);
                                } else {
                                    return raise("A variable declaration must be enclosed with a pair of '<' and '>'.");
                                }
                            }
                        } else {
                            return raise("Unclosed '[' when declaring flag.");
                        }
                        ended = true;
                        continue;
                    }
                    if (ended) [[unlikely]] {
                        return raise("Declaring non-flag attributes after declaring the first flag.");
                    }
                    if (t.starts_with(Config.specials.var_open)) {
                        std::size_t var_start = Config.specials.var_open.size(),
                        var_end = t.find(Config.specials.var_close, var_start);
                        if (var_end == t.npos) [[unlikely]] {
                            return raise("Unclosed < when declaring variable.");
                        }
                        const std::size_t var_index = add_var(t.substr(var_start, var_end - var_start));
                        var_end += Config.specials.var_close.size();
                        const parse_node_type type = (var_end == t.size()) ? variable : variable_option;
                        std::vector<string_view_type> prev_opts;
                        bool insert = false;
                        if (searching) {
                            while (tree[current].type == option || tree[current].type == variable_option) {
                                if (type == variable_option) {
                                    prev_opts.emplace_back(tree[current].option_name);
                                }
                                if (tree[current].next_placeholder) {
                                    current = tree[current].next_placeholder;
                                } else {
                                    break;
                                }
                            }
                            switch (tree[current].type) {
                                case option:
                                case variable_option: {
                                    tree[current].next_placeholder = tree.size();
                                    break;
                                }
                                case variable: {
                                    string_view_type var_name = var_names[tree[current].var_index];
                                    while (tree[current].next) {
                                        current = tree[current].next;
                                        while (tree[current].next_placeholder) {
                                            current = tree[current].next_placeholder;
                                        }
                                    }
                                    std::size_t usage_index = tree[current].usage_index;
                                    std::array<define_error_ref, 2> refs = {
                                        get_error("Attempt to declare two variables on the same position."),
                                        {
                                            .what = "Previous variable declared here.",
                                            .usage_index = usage_index,
                                            .loc = var_name.data() - Config.usages[usage_index].format.data(),
                                            .type = note
                                        }
                                    };
                                    return std::unexpected(define_error{Config, refs});
                                }
                                case variadic:
                                case end: {
                                    insert = true;
                                    break;
                                }
                            }
                        }
                        const std::size_t added_first = tree.size();
                        if (type == variable) {
                            tree.push_back({
                                .type = variable,
                                .var_index = var_index,
                                .next = tree.size() + 1
                            });
                        } else {
                            t.remove_prefix(var_end);
                            if (!t.starts_with(Config.specials.equal)) [[unlikely]] {
                                return raise("Unexpected character after declaration of variable.");
                            }
                            t.remove_prefix(Config.specials.equal.size());
                            if (!t.starts_with(Config.specials.compound_open)) [[unlikely]] {
                                return raise("Expected '(' for declaration of a variable option.");
                            }
                            ranges::sort(prev_opts);
                            if (auto res = add_option(variable_option, var_index, prev_opts)) [[unlikely]] {
                                return *res;
                            }
                        }
                        if (insert) insert_before_end(added_first);
                        searching = false;
                    } else if (t == Config.specials.variadic) {
                        end_type = variadic;
                        searching = false;
                        break;
                    } else {
                        if (searching) {
                            while (true) {
                                switch (tree[current].type) {
                                    case option:
                                    case variable_option: {
                                        if (tree[current].option_name == t) {
                                            current = tree[current].next;
                                        } else if (tree[current].next_placeholder) {
                                            current = tree[current].next_placeholder;
                                            continue;
                                        } else {
                                            tree[current].next_placeholder = tree.size();
                                            if (auto res = add_option()) return *res;
                                            searching = false;
                                        }
                                        break;
                                    }
                                    case variable:
                                    case variadic:
                                    case end: {
                                        const std::size_t added_first = tree.size();
                                        if (auto res = add_option()) return *res;
                                        insert_before_end(added_first);
                                        searching = false;
                                        break;
                                    }
                                }
                                break;
                            }
                        } else {
                            if (auto res = add_option()) return *res;
                        }
                    }
                }
                if (searching) {
                    while (tree[current].next_placeholder) {
                        current = tree[current].next_placeholder;
                    }
                    tree[current].next_placeholder = tree.size();
                }
                if (!tree.empty()) [[likely]] {
                    parse_node_type type = tree[current].type;
                    std::string_view what;
                    std::size_t offset = 0;
                    if (type == variadic) [[unlikely]] {
                        what = "Another usage takes variadic arguments in this position.";
                        offset = -Config.specials.variadic.size();
                    }
                    if (type == end) [[unlikely]] {
                        what = "Another usage ends in this position.";
                    }
                    if (what.data()) {
                        std::size_t usage_index = tree[current].usage_index;
                        std::array<define_error_ref, 2> refs = {get_error(what, t.size() + offset), {
                            .what = "Previously defined here.",
                            .usage_index = usage_index,
                            .loc = Config.usages[usage_index].format.size() + offset,
                            .type = note
                        }};
                        return std::unexpected(define_error{Config, refs});
                    }
                }
                tree.push_back({.type = end_type, .usage_index = static_cast<std::size_t>(i)});
#undef RAISE
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
    }
    template <config_instance auto& Config>
    using config_type_of = std::remove_cvref_t<decltype(Config)>::super_type;
    /// \tparam Config: parser configuration (in `config::type`)
    /// \tparam FlagSetSize: size of hash set for flags
    /// \tparam Hash: hasher (of `string_view_type`)
    /// \return `parser_info`
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
        constexpr auto res = detail::parse_usage<Config, char_type, Hash, FlagSetSize>(nullptr);
        if constexpr (res) {
            parser_info<*res, Hash> info;
            detail::parse_usage<Config, char_type, Hash, FlagSetSize>(&info);
            return info;
        } else {
#if __cpp_static_assert >= 202306L
            static_assert(false, res.error());
#endif
        }
    }
}

template <cmd::tagged<cmd::detail::usage_range_tag> Rng>
struct std::formatter<Rng, typename Rng::format_char_type> {
    using char_type = Rng::char_type;
    using format_char_type = Rng::format_char_type;
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }
    constexpr auto format(const Rng& rng, auto& ctx) const {
        auto out = ctx.out();
        for (const auto usage : rng.value) {
            out = std::format_to(out, Rng::format, cmd::translator<char_type, format_char_type>{}(usage->format));
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
            auto out = std::format_to(ctx.out(), config.usage_tmpl, ref.template wrap<0>());
            return std::format_to(out, config.usage_tmpl, ref.template wrap<1>());
        }
    }
};

template <cmd::tagged<cmd::error_ref_c_tag> RefC>
struct std::formatter<RefC, typename RefC::super_type::format_char_type> {
    using parser_type = RefC::super_type;
    using char_type = parser_type::char_type;
    using format_char_type = parser_type::format_char_type;
    using string_view_type = parser_type::string_view_type;
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }
    constexpr auto format(const RefC& ref_c, auto& ctx) const {
        auto out = ctx.out();
        const auto& ref = ref_c.ref;
        constexpr const auto& specials = parser_type::config.specials;
        bool first = true, indicated = false;
        std::size_t i = 0;
        constexpr cmd::translator<char_type, format_char_type> t{};
        for (string_view_type arg : ref.args) {
            if (first) {
                first = false;
            } else {
                out = ranges::copy(t({&specials.delimiter, 1}), out).out;
            }
            if constexpr (RefC::mode == 0) {
                out = ranges::copy(t(arg), out).out;
            } else if constexpr (RefC::mode == 1) {
                for (std::size_t j = 0; j < arg.size(); ++j) {
                    if ((i == ref.loc.arg_loc) && (j == ref.loc.in_arg_loc)) [[unlikely]] {
                        out = ranges::copy(t({&specials.indicator, 1}), out).out;
                        indicated = true;
                    } else {
                        out = ranges::copy(t({&specials.delimiter, 1}), out).out;
                    }
                }
            }
            ++i;
        }
        if ((RefC::mode == 1) && !indicated) {
            out = ranges::copy(t({&specials.indicator, 1}), out).out;
        }
        return out;
    }
};