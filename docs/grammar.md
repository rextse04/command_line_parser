# Usage Text Grammar
Every valid usage is of the form ```<arg1> <arg2> ... <argn>```,
where each ```<argi>``` must be one of the following:
1. Option. Any ```<argi>``` that does not start with a special character sequence is an option.
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
5. Variadic variable. ```<argi>``` is a variadic variable if it is exactly ```config.specials.variadic```.
It accepts any number of positional arguments (including 0).
Naturally, only flags can follow a variadic variable,
and two variadic variables cannot be declared in the same position.
6. Flag. A flag is of the form ```[--flag[=<var>]]```.
   Flags are non-positional - they can be placed in any order in a command,
   as long as they are put after all positional arguments.
   If ```=<var>``` is present, an argument is required and captured in ```<var>```,
   otherwise ```--flag``` is a boolean flag.

All other strings are ill-formed.