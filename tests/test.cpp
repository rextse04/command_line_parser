#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE Main Test
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <parser.hpp>
#include <config_default.hpp>
#include <print>
#include <tuple>
#include <codecvt>

#define CONCAT2(x, y) x ## y
#define CONCAT(x, y) CONCAT2(x, y)
#define BOOST_ANON_TEST_CASE(...) BOOST_AUTO_TEST_CASE(CONCAT(test_, __LINE__) __VA_OPT__(,) __VA_ARGS__)

constexpr cmd::config<int>::type config{
    .name = "Test application",
    .description = "Test description.",
    .usages = {
        {"test arg1 arg2 [--test_flag=<var>]", 1},
        {"test (arg3|arg4) (arg5|arg6) <var> [--test_flag=<var2>] [--bool_flag]", 2},
        {"test <var>=(arg7|arg8) <var2>=(arg9|arg10|...) ...", 3},
        {"", 4}
    }
};
constexpr auto info = cmd::define_parser<config>();
constinit cmd::parser<info> parser;
struct parse_tests_fixture {
    void teardown() {
        parser.reset();
    }
};
BOOST_FIXTURE_TEST_SUITE(parse_tests, parse_tests_fixture)
using enum cmd::error_type;
BOOST_ANON_TEST_CASE() {
    parser.print_man();
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg1  arg2 --test_flag=\"test var\"";
    auto res = parser.parse(input);
    BOOST_REQUIRE(res.has_value());
    BOOST_CHECK_EQUAL(res->result, 1);
    BOOST_CHECK(parser.flag("--test_flag"));
    BOOST_CHECK_EQUAL(parser.var("var"), "test var");
    parser.raise_argument_error(*res, "var", "no good.").print();
    std::println();
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = R"(test arg3 arg6 "\\test var")";
    auto res = parser.parse(input);
    BOOST_REQUIRE(res.has_value());
    BOOST_CHECK_EQUAL(res->result, 2);
    BOOST_CHECK_EQUAL(parser.var("var"), "\\test var");
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg3 arg4 var";
    auto res = parser.parse(input);
    BOOST_REQUIRE(!res.has_value());
    BOOST_CHECK(res.error().type == unknown_option);
    std::println("{}", res.error());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg3 arg5";
    auto res = parser.parse(input);
    BOOST_REQUIRE(!res.has_value());
    BOOST_CHECK(res.error().type == too_few_arguments);
    std::println("{}", res.error());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg3 arg5 var var2";
    auto res = parser.parse(input);
    BOOST_REQUIRE(!res.has_value());
    BOOST_CHECK(res.error().type == too_many_arguments);
    std::println("{}", res.error());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg3 arg5 -var";
    auto res = parser.parse(input);
    BOOST_REQUIRE(!res.has_value());
    BOOST_CHECK(res.error().type == flag_cannot_be_variable);
    std::println("{}", res.error());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg1 arg2 --unknown_flag";
    auto res = parser.parse(input);
    BOOST_REQUIRE(!res.has_value());
    BOOST_CHECK(res.error().type == unknown_flag);
    std::println("{}", res.error());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg3 arg5 var --bool_flag=var";
    auto res = parser.parse(input);
    BOOST_REQUIRE(!res.has_value());
    BOOST_CHECK(res.error().type == flag_does_not_accept_argument);
    std::println("{}", res.error());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg7 arg11";
    auto res = parser.parse(input);
    BOOST_REQUIRE(res.has_value());
    BOOST_CHECK_EQUAL(res->result, 3);
    BOOST_CHECK(parser.variadic().empty());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test arg8 arg9 arg10 arg11 arg12";
    auto res = parser.parse(input);
    BOOST_REQUIRE(res.has_value());
    BOOST_CHECK_EQUAL(res->result, 3);
    std::vector<std::string> variadic{"arg10", "arg11", "arg12"};
    BOOST_CHECK_EQUAL_COLLECTIONS(
        parser.variadic().begin(), parser.variadic().end(),
        variadic.begin(), variadic.end());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "test unknown_arg arg9";
    auto res = parser.parse(input);
    BOOST_REQUIRE(!res.has_value());
    BOOST_CHECK(res.error().type == unknown_option);
    std::println("{}", res.error());
}
BOOST_ANON_TEST_CASE() {
    std::string_view input = "";
    auto res = parser.parse(input);
    BOOST_CHECK(res.has_value());
    BOOST_CHECK_EQUAL(res->result, 4);
    BOOST_CHECK_EQUAL(parser.var("var"), "");
    BOOST_CHECK(!parser.flag("--test_flag"));
}
BOOST_AUTO_TEST_SUITE_END()

#define DEFINE_CHECKS_CASE(...) BOOST_ANON_TEST_CASE() {\
    static constexpr cmd::config<int, CharT, CharT>::type config{__VA_ARGS__};\
    auto res = cmd::detail::parse_usage<config, CharT, Hash, FlagSetSize>(nullptr);\
    BOOST_REQUIRE(!res.has_value());\
    std::println("{}\n", res.error().what());\
}
BOOST_AUTO_TEST_SUITE(define_tests)
using CharT = char;
using Hash = cmd::hash<std::string_view>;
constexpr size_t FlagSetSize = 512;
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test  arg1", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test arg1 [[flag]]", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test arg1 [--flag] [--flag]", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test arg1", 1}, {"test arg1" ,2}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"", 1}, {"" ,2}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test ()", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test (|)", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test arg1|arg2", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test (arg1|arg2", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test arg1", 1}, {"test <var>=(arg2|arg1)", 2}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test <var>", 1}, {"test <var2>=(arg1|arg2|...)", 2}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test <var><var2>", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test <var>=(arg1|arg2", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test <var>=(arg1|...|arg2)", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test (arg1|arg2|...)", 1}}
)
DEFINE_CHECKS_CASE(
    .name = "Test application",
    .usages = {{"test <var>=arg1", 1}}
)
BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(wchar_tests)
using CharT = wchar_t;
using Hash = cmd::hash<std::wstring_view>;
constexpr size_t FlagSetSize = 512;
DEFINE_CHECKS_CASE(
    .name = L"Test application",
    .usages = {{L"test  arg1", 1}}
)
BOOST_AUTO_TEST_SUITE_END()