# Defining a parser on character types other than ```char```
By default, the library supports two combinations of input and output character types:
1. ```char_type=char``` and ```format_char_type=char```
2. ```char_type=wchar``` and ```format_char_type=wchar```

Note that due to the limitations of the standard library,
```format_char_type``` can only be ```char``` or ```wchar```.
To define a ```parser``` on different combinations, follow the steps below:
1. Make sure ```cmd::outputtable<char_type, format_char_type>``` is fulfilled.
You may need to specialize ```std::formatter``` for this.
2. To set default ```config``` fields for your combination,
specialize ```cmd::config_default```.
Check out the convenience macro ```CONFIG_DEFAULT```.
3. Check the type constraints of ```cmd::hash``` and
determine if you need to supply a custom hasher.
4. (Optional) Register default I/O streams by specializing
```cmd::input_object``` and ```cmd::output_object```.