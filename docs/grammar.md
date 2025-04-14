# Usage Text Grammar
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