#include <complex>
#include <unordered_map>
#include <optional>
#include <iostream>
#include <print>
#include <spanstream>
#include <cmath>
#include <limits>
#include <locale>
#include <parser.hpp>
#include <config_default.hpp> // for default diagnostic messages

static_assert(std::numeric_limits<double>::is_iec559, "This program requires IEEE754 standard.");

enum class action {
    arithmetic, sqrt, save, read, help, exit
};
constexpr auto config = []() {
    cmd::config<action, char16_t, char>::type config{
        .name = "Calculator",
        .description = "A simple CLI calculator that supports complex numbers.",
        .usages = {
            {u"<op>=(add|minus|mul|div) <first> <second>", action::arithmetic},
            {u"sqrt <first> [--real] [--ℝ]", action::sqrt},
            {u"save <var> [--value=<value>]", action::save},
            {u"read <var>", action::read},
            {u"help", action::help},
            {u"exit", action::exit}
        },
        .explanation = "\033[36mDescription:\033[0m\n"
            "\033[1mBasic arithmetic\033[0m\n"
            "Syntax: <op> <first> <second>\n"
            "Computes \"<first> <op> <second>\". <first> and <second> can be (complex) numbers or variable names.\n"
            "\033[1mAdvanced arithmetic\033[0m\n"
            "Syntax: sqrt <first> [--complex]\n"
            "Computes the square root of <first>. <first> can be a (complex) number or a variable name."
            "If --real is present, NaN is returned if sqrt(<first>) returns a non-real number.\n"
            "\033[1mMemory\033[0m\n"
            "Syntax: save <var> [--value=<value>]\n"
            "Save <value> to variable <var>. If --value is missing, result from the most recent step will be saved."
            "Syntax: read <var>\n"
            "Read variable <var>.\n"
            "\033[1mMisc.\033[0m\n"
            "Syntax: help\n"
            "Prints this text.\n"
            "Syntax: exit\n"
            "Exit the program."
    };
    // we change the default of flag_prefix so we can allow negative numbers as arguments
    config.specials.flag_prefix = u"--";
    return config;
}();
constexpr auto info = cmd::define_parser<config>();
constinit cmd::parser<info> parser;

using number_t = std::complex<double>;
std::unordered_map<std::u16string, number_t> vars;
std::optional<number_t> result;
// res: std::expected<parse_result, parse_error>
int run(const auto& res) {
    using enum action;
    if (res) {
        bool success = true;
        auto to_number = [&res, &success](cmd::parser<info>::var_name name, number_t& number) -> void {
            std::u16string arg{parser.var(name)};
            if (vars.contains(arg)) {
                number = vars[arg];
                return;
            }
            auto carg = cmd::translator<char16_t, char>{}(arg);
            std::ispanstream ss(carg);
            ss >> number;
            if (ss.fail()) {
                parser.raise_argument_error(*res, name, "Invalid number.").print();
                success = false;
            }
        };
        switch (res->result) {
            case arithmetic: {
                std::u16string_view op = parser.var(u"op");
                number_t first, second;
                to_number(u"first", first);
                to_number(u"second", second);
                if (!success) return EXIT_FAILURE;
                if (op == u"add") {
                    result = first + second;
                } else if (op == u"minus") {
                    result = first - second;
                } else if (op == u"mul") {
                    result = first * second;
                } else if (op == u"div") {
                    result = first / second;
                } else {
                    std::unreachable();
                }
                break;
            }
            case sqrt: {
                number_t first;
                to_number(u"first", first);
                if (!success) return EXIT_FAILURE;
                result = std::sqrt(first);
                if (parser.flag(u"--real") || parser.flag(u"--ℝ")) {
                    if (result->imag() != 0) result = {NAN, NAN};
                }
                break;
            }
            case save: {
                if (parser.flag(u"--value")) {
                    number_t number;
                    to_number(u"value", number);
                    if (!success) return EXIT_FAILURE;
                    result = number;
                }
                if (result) {
                    vars[std::u16string{parser.var(u"var")}] = *result;
                } else {
                    std::println("Error: No result from previous steps!");
                }
                return EXIT_SUCCESS;
            }
            case read: {
                if (!vars.contains(std::u16string{parser.var(u"var")})) {
                    std::println("Error: Unknown variable.");
                    return EXIT_FAILURE;
                }
                result = vars[std::u16string{parser.var(u"var")}];
                break;
            }
            case help: {
                parser.print_man();
                std::println();
                return EXIT_SUCCESS;
            }
            case exit: {
                return -1;
            }
        }
        std::cout << *result << '\n';
    } else {
        res.error().print();
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    using enum action;
    std::locale system_locale("");
    std::locale::global(system_locale);
    if (argc <= 1) {
        while (true) {
            std::print(">> ");
            auto res = parser.readline();
            if (run(res) < 0) return EXIT_SUCCESS;
            parser.reset();
        }
    } else {
        auto res = parser.parse(argc - 1, argv + 1);
        return run(res);
    }
}