# cmd-args

A lightweight, header-only C++ library for parsing command-line arguments.

## Features
 - Header-only: Just add `cmd-args.hpp` to your project and `#include` it!
 - Simple, intuitive syntax
 - Supports C++ 11 onwards
 - Ultra-small - just over 200 lines! (not including comments and whitespace)

## How To Use cmd-args

### Quick(ish)start

Once you've included the header file in your project, the first step is to create an `ArgParser` object, which (along with everything else in cmd-args) is part of the `CmdArgs` namespace. Then you can start adding arguments to the parser, which is done by using `parser.add<ArgType>(args)` to add one of the three argument objects:

- **Value Arguments** (`ValueArg<T>`) hold a value of type `T`, which is set by naming them in the command, followed by a value of type `T`. They are constructed with a short name, a long name, and a description (all `std::string`), and they can also optionally be given a default value of type `T`.
- **Implicit Arguments** (`ImplicitArg<T>`) also hold a value of type `T`. If they are named in the command and followed by a value of type `T`, they are set to that value, otherwise they are set to a default value. They are constructed with a short name, a long name, and a description (all `std::string`), along with a default value of type `T`. They can also optionally be given a second value of type `T`, which is their value if they are not named in the command at all.
- **Flag Arguments** (`FlagArg`) hold a boolean, which is true if they are named in the command, and false if they are not. They are constructed with a short name, a long name, and a description (all `std::string`).

Note: To make arguments follow the standard format, the `Argument` constructor adds one dash to the front of the short name, and two dashes to the beginning of the long name. Make sure you don't have those in the name, or they'll be doubled.

The `add` method also returns a `std::shared_ptr` to the object, so you'll want to store that somewhere. After you've added all your arguments, simply call `parser.parseCmd(int argc, char **argv)` on the program's command-line input:

```c++
#include "cmd-args.hpp"
using namespace CmdArgs;
int main(int argc, char **argv) {
  ArgParser parser;
  
  // add arguments here...
  
  parser.parseCmd(argc, argv);
  
  // do stuff with arguments here...
}
```

Now that the command is parsed, you can get the value of any added arguments by calling `Argument->value()`, which returns a value of the same type as the argument. For example:
```c++
----Example.cpp---
#include <iostream>
#include "cmd-args.hpp"
using namespace CmdArgs;

int main (int argc, char **argv) {
  ArgParser parser; // create argument parser
  
  // add arguments
  auto value1 = parser.add<ValueArg<int>>("v", "value1", "a value argument with no default value");
  auto value2 = parser.add<ValueArg<int>>("", "value2", "a value argument with a default value", 42);
  auto implicit1 = parser.add<ImplicitArg<int>>("i", "implicit1", "an implicit argument", 37);
  auto implicit2 = parser.add<ImplicitArg<std::string>>("", "implicit2", "a string implit argument", "hello world!");
  auto implicit3 = parser.add<ImplicitArg<int>>("", "implicit3", "an implicit argument with a default value", 438, 19);
  auto flag1 = parser.add<FlagArg>("f", "flag1", "a flag argument");
  auto flag2 = parser.add<FlagArg>("", "flag2", "another flag argument");
  
  parser.parseCmd(argc, argv); // parse the command input
  
  // print out the value of each argument
  std::cout << "value 1: " << value1->value() << '\n';
  std::cout << "value 2: " << value2->value() << '\n';
  std::cout << "implicit 1: " << implicit1->value() << '\n';
  std::cout << "implicit 2: " << implicit2->value() << '\n';
  std::cout << "implicit 3: " << implicit3->value() << '\n';
  std::cout << "flag 1: " << flag1->value() << '\n';
  std::cout << "flag 2: " << flag2->value() << '\n';
}
```
Would produce this output (if the command input was the same)
```
./Example.exe -v 53 -i --implicit2 "hi!" --flag2
value 1: 53
value 2: 42
implicit 1: 37
implicit 2: hi!
implicit 3: 19
flag 1: 0
flag 2: 1
```

### Help Messages

The `ArgParser` can also automatically create a help message for all of its arguments. Just call `ArgParser.createHelpMessage()`, and you'll get a formatted table with each command's name(s), description, and default value (if it has one). The commands are ordered in the same order that they were added to the parser. For example:

```c++
----Example.cpp---
#include <iostream>
#include "cmd-args.hpp"
using namespace CmdArgs;

int main (int argc, char **argv) {
  ArgParser parser; // create argument parser
  
  // add arguments
  auto value1 = parser.add<ValueArg<int>>("v", "value1", "a value argument with no default value");
  auto value2 = parser.add<ValueArg<int>>("", "value2", "a value argument with a default value", 42);
  auto implicit1 = parser.add<ImplicitArg<int>>("i", "implicit1", "an implicit argument", 37);
  auto implicit2 = parser.add<ImplicitArg<std::string>>("", "implicit2", "a string implit argument", "hello world!");
  auto implicit3 = parser.add<ImplicitArg<int>>("", "implicit3", "an implicit argument with a default value", 438, 19);
  auto flag1 = parser.add<FlagArg>("f", "flag1", "a flag argument");
  auto flag2 = parser.add<FlagArg>("", "flag2", "another flag argument");
  
  std::cout << parser.createHelpMessage();
}
```

Produces:

```
[[Allowed Arguments]]
  -v, --value1                         a value argument with no default value
    , --value2 =42                     a value argument with a default value
  -i, --implicit1 =arg(=37)            an implicit argument
    , --implicit2 =arg(=hello world!)  a string implit argument
    , --implicit3 =arg(=438)           an implicit argument with a default value
  -f, --flag1                          a flag argument
    , --flag2                          another flag argument
```
Note: If an implicit argument has a default value (the second one in the constructor), the help message won't display it. It probably will in the future.

### Hidden Arguments
You can set an argument's visibility - which determines whether it appears in help messages - by including it as the first parameter in the `add()` method:
- `VISIBLE` arguments always appear in help messages
- `HIDDEN` arguments don't appear in help messages unless the `showHidden` parameter in `createHelpMessage` is true
- `INVISIBLE` arguments never appear in help messages

If you don't set the argument's visibility when you add it, it defaults to `VISIBLE`. For example:

```c++
#include <iostream>
#include "cmd-args.hpp"
using namespace CmdArgs;

int main(int argc, const char **argv) {
  ArgParser parser;

  auto visible = parser.add<ValueArg<std::string>>("v", "visible", "I'm over here!");
  auto hidden = parser.add<ValueArg<std::string>>(HIDDEN, "h", "hidden", "They'll never find me here...");
  auto invisible = parser.add<ValueArg<std::string>>(INVISIBLE, "i", "invisible", "You think the shadows are your ally?");

  std::cout << "Without Hidden Arguments:\n";
  std::cout << parser.createHelpMessage() << '\n';
  std::cout << "With Hidden Arguments:\n";
  std::cout << parser.createHelpMessage(true) << '\n';
}
```
Produces:
```
Without Hidden Arguments:                      
[[Allowed Arguments]]                          
  -v, --visible   I'm over here!               
                                               
With Hidden Arguments:                         
[[Allowed Arguments]]                          
  -v, --visible   I'm over here!               
[[Hidden Arguments]]                           
  -h, --hidden    They'll never find me here...
```

### Other Argument Methods
In addition to `value()`, arguments also have a few other methods:
- `isSet()` returns whether the argument was set in the command
- `isValid()` returns whether the argument's value is defined (from a default value or from being set in the command)
- `defaultSetValue()` (only for `ImplicitArg`) returns what the argument is set to if its name appears in the command without a value
- `hasDefault()` (only for `ValueArg` and `ImplicitArg`) returns whether the argument has a default value (for `ImplicitArg`, this is the value used if the argument isn't in the command at all)
- `defaultValue()` (only for `ValueArg` and `ImplicitArg`) returns the argument's default value (causes undefined behavior if the default value was never set)

The short name, long name, description, and visibility are constants, so you can get them using the `shortName`, `longName`, `description`, and `visibility` members, respectively.