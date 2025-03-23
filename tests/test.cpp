#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE Main Test
#include <boost/test/unit_test.hpp>
#include <boost/mpl/list.hpp>
#include <parser.hpp>
#include <config_default.hpp>
#include <print>

BOOST_AUTO_TEST_CASE(parse_checks) {
    using enum cmd::error_type;
    static constexpr cmd::config<int>::type config{
        .name = "Test application",
        .usages = {
            {"test arg1 arg2 [--test_flag=<var>]", 1},
            {"test (arg3|arg4) (arg5|arg6) <var> [--test_flag=<var2>] [--bool_flag]", 2},
            {"", 3}
        }
    };
    static constexpr auto info = cmd::define_parser<config>();
    static constinit cmd::parser<info> parser;

    parser.print_man();

    {
        std::string_view input = "test arg1  arg2 --test_flag=\"test var\"";
        auto res = parser.parse(input);
        BOOST_CHECK(res.has_value());
        BOOST_CHECK_EQUAL(res->result, 1);
        BOOST_CHECK(parser.flag("--test_flag"));
        BOOST_CHECK_EQUAL(parser.var("var"), "test var");
        parser.raise_argument_error(*res, "var", " no good.").print();
        std::println();
    }

    {
        parser.reset();
        std::string_view input = "test arg3 arg6 \"\\\\test var\"";
        auto res = parser.parse(input);
        BOOST_CHECK(res.has_value());
        BOOST_CHECK_EQUAL(res->result, 2);
        BOOST_CHECK_EQUAL(parser.var("var"), "\\test var");
    }

    {
        parser.reset();
        std::string_view input = "test arg3 arg4 var";
        auto res = parser.parse(input);
        BOOST_CHECK(!res.has_value());
        BOOST_CHECK(res.error().type == unknown_option);
        std::print("{}", res.error());
        std::println();
    }

    {
        parser.reset();
        std::string_view input = "test arg3 arg5";
        auto res = parser.parse(input);
        BOOST_CHECK(!res.has_value());
        BOOST_CHECK(res.error().type == too_few_arguments);
        std::print("{}", res.error());
        std::println();
    }

    {
        parser.reset();
        std::string_view input = "test arg3 arg5 var var2";
        auto res = parser.parse(input);
        BOOST_CHECK(!res.has_value());
        BOOST_CHECK(res.error().type == too_many_arguments);
        std::print("{}", res.error());
        std::println();
    }

    {
        parser.reset();
        std::string_view input = "test arg3 arg5 -var";
        auto res = parser.parse(input);
        BOOST_CHECK(!res.has_value());
        BOOST_CHECK(res.error().type == flag_cannot_be_variable);
        std::print("{}", res.error());
        std::println();
    }

    {
        parser.reset();
        std::string_view input = "test arg1 arg2 --unknown_flag";
        auto res = parser.parse(input);
        BOOST_CHECK(!res.has_value());
        BOOST_CHECK(res.error().type == unknown_flag);
        std::print("{}", res.error());
        std::println();
    }

    {
        parser.reset();
        std::string_view input = "test arg3 arg5 var --bool_flag=var";
        auto res = parser.parse(input);
        BOOST_CHECK(!res.has_value());
        BOOST_CHECK(res.error().type == flag_does_not_accept_argument);
        std::print("{}", res.error());
        std::println();
    }

    {
        parser.reset();
        std::string_view input = "";
        auto res = parser.parse(input);
        BOOST_CHECK(res.has_value());
        BOOST_CHECK_EQUAL(res->result, 3);
        BOOST_CHECK_EQUAL(parser.var("var"), "");
        BOOST_CHECK(!parser.flag("--test_flag"));
    }
}

#define STATIC_CHECKS_CASE(...) {\
    static constexpr cmd::config<int, CharT, CharT>::type config{__VA_ARGS__};\
    static constexpr auto res = cmd::_parse_usage<config, CharT, Hash, FlagSetSize>(nullptr);\
    BOOST_CHECK(!res.has_value());\
    if (!res) {\
        std::println("{}\n\n", res.error().what());\
    }\
}
BOOST_AUTO_TEST_CASE(static_checks) {
    using CharT = char;
    using Hash = cmd::hash<std::string_view>;
    static constexpr size_t FlagSetSize = 512;
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test  arg1", 1}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test arg1", 1}, {"test <var>", 2}, {"test arg2", 3}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test arg1 [[flag]]", 1}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test arg1 [--flag] [--flag]", 1}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test arg1", 1}, {"test arg1" ,2}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"", 1}, {"" ,2}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test (|)", 1}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test arg1|arg2", 1}}
    )
    STATIC_CHECKS_CASE(
        .name = "Test application",
        .usages = {{"test (arg1|arg2", 1}}
    )
}

BOOST_AUTO_TEST_CASE(wchar_test) {
    {
        using enum cmd::error_type;
        static constexpr cmd::config<int, wchar_t, wchar_t>::type config{
            .name = L"Test application",
            .usages = {
                    {L"test arg1 arg2 [--test_flag=<var>]", 1},
                    {L"test (arg3|arg4) (arg5|arg6) <var> [--test_flag=<var2>] [--bool_flag]", 2},
                    {L"", 3}
            }
        };
        static constexpr auto info = cmd::define_parser<config>();
        static constinit cmd::parser<info> parser;

        parser.print_man();

        {
            std::wstring_view input = L"test arg1  arg2 --test_flag=\"test var\"";
            auto res = parser.parse(input);
            BOOST_CHECK(res.has_value());
            BOOST_CHECK_EQUAL(res->result, 1);
            BOOST_CHECK(parser.flag(L"--test_flag"));
            BOOST_CHECK(parser.var(L"var") == L"test var");
        }

        {
            parser.reset();
            std::wstring_view input = L"test arg3 arg6 \"\\\\test var\"";
            auto res = parser.parse(input);
            BOOST_CHECK(res.has_value());
            BOOST_CHECK_EQUAL(res->result, 2);
            BOOST_CHECK(parser.var(L"var") == L"\\test var");
        }

        {
            parser.reset();
            std::wstring_view input = L"test arg3 arg4 var";
            auto res = parser.parse(input);
            BOOST_CHECK(!res.has_value());
            BOOST_CHECK(res.error().type == unknown_option);
            res.error().print();
            std::println();
        }
    }

    using CharT = wchar_t;
    using Hash = cmd::hash<std::wstring_view>;
    static constexpr size_t FlagSetSize = 512;
    STATIC_CHECKS_CASE(
        .name = L"Test application",
        .usages = {{L"test  arg1", 1}}
    )
}