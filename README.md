# Command Line Parser
A simple command-line parser inspired by [docopt](https://github.com/docopt/),
except the parser is generated at compile time using C++23 features 🤗.
Instead of convoluted classes and functions,
you define the parser just as you write the manual -
this header-only library does the heavy lifting for you.

## Quick Example
For a working standalone example, see [this](example/main.cpp).
```c++
#include <parser.hpp>
#include <config_default.hpp> // for default diagnostic messages
enum class action {
    arithmetic, sqrt, set
};
constexpr cmd::config<action>::type config{
    .name = "Calculator",
    .description = "A simple CLI calulator.",
    .usages = {
        {"<op>=(add|minus|mul|div) <first> <second>", action::arithmetic},
        {"sqrt <first> [--real]", action::sqrt},
        {"save <var> [--value=<value>]", action::save},
        {"read <var>", action::read},
        {"help", action::help}
    }
};
constexpr auto info = cmd::define_parser<config>();
constinit cmd::parser<info> parser;
```
```config``` stores the configuration of the parser,
```info``` contains the generated parse tree based on ```config```,
and ```parser``` is the actual parser object you interact with.

To parse a command, you have a few options:

| Scenario                                                                                      | Code                           |
|-----------------------------------------------------------------------------------------------|--------------------------------|
| You have a forward range ```rng``` of arguments (in ```std::basic_string_view<char_type>```). | ```parser.parse(rng)```        |
| You have a forward range ```rng``` of characters that form a command.                         | ```parser.parse(rng)```        |
| You wish to directly parse ```argc, argv```.                                                  | ```parser.parse(argc, argv)``` |
| You wish to parse a line from ```std::cin```.                                                 | ```parser.readline()```        |

All method calls return an object ```res``` of type ```std::expected<parse_result<...>, parse_error<...>>```.
If parsing is successful, the object contains a ```parse_result```:
- ```res->result``` is the usage chosen. In this case, it would be of type ```action```.
- ```res->usage_index``` is the index of the usage chosen.
- ```res->args``` is a range of arguments passed to the parser.
Depending on your usage, ```res``` may or may not own the range.
If it does not, ```res``` is not copyable nor movable.

Otherwise, if parsing is unsuccessful, ```res``` contains a ```parse_error```:
- ```res.error().type``` (of ```error_type```) is the type of the error.
- ```res.error().ref``` gives the location of the error in the input.
Similar to above, depending on your usage, it may or may not own the input.
It it does not, ```res``` is not copyable nor movable.
- ```res.error().refs``` is a vector of related ```usage```s.
- ```res.error().print()``` prints a human-readable error message.

To access variables and flags read during parsing,
call ```parser.var("var_name")``` and ```parser.flag("--flag_name")```.
The former returns a ```std::basic_string_view<char_type>```,
while latter returns a ```bool``` indicating whether the flag has been set.

If the value passed to a variable is erroneous, you can raise an ```argument_error```
by calling ```res.raise_argument_error(res, "var_name", "error message")```.
The benefit of this over printing your own error message is that
it provides the error location and related messages for free 😋, like the following:
```
Error: Invalid argument: Invalid number.
| add 1 two
|       ^  
Closest usages:
| <op>=(add|minus|mul|div) <first> <second>
```

Finally, to parse another command, call ```parser.reset()```,
otherwise variables and flags from the previous parse will be retained.

## Usage Text Grammar
Every valid usage is of the form ```<arg1> <arg2> ...```,
where each ```<argi>``` must be one of the following:
1. Option. Any string that does not start with a special character sequence is an option.
Options are positional arguments and must be exactly matched.
2. Compound option. If ```<argi>``` is enclosed by
```config.specials.compound_open``` (default: ```(```) and
```config.specials.compound_close``` (default: ```)```), it is a compound option.
Compound options are simply a convenient way to put multiple options at the same position.
Possible options are separated by ```config.specials.compound_divider``` (default:```|```).
3. Variable. Variables are enclosed by ```config.specials.var_open``` (default: ```<```) and
```config.specials.var_close``` (default: ```>```).
No two variables can be put at the same position.
4. Variable option. Variables options are of the form ```<var>=(opt1|opt2[|...])```.
If ```config.specials.var_capture``` (default: ```...```) is present,
```<var>``` is an ordinary variable - the options are simply for hinting purposes.
Otherwise, ```<var>``` only accepts the listed options.
5. Flag. A flag is of the form ```[--flag[=<var>]]```.
Flags are non-positional - they can be placed in any order in a command,
as long as they are put after all positional arguments.
If ```=<var>``` is present, an argument is required and captured in ```<var>```,
otherwise ```--flag``` is a boolean flag.

All other strings are ill-formed.