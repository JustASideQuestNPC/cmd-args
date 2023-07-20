#pragma once
#include <sstream>
#include <unordered_map>
#include <memory>
#include <type_traits>
#include <iostream>

namespace CmdArgs {
  std::string lowerString(const std::string str) {
    std::string lowered{str};
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](char c){ return std::tolower(c); });
    return lowered;
  }

  template <typename T>
  T stringToType(const std::string str) {
    std::istringstream convert{str};
    T val;
    if (convert >> val) return val;
    else throw std::invalid_argument("Couldn't convert string \"" + str + "\" to type " + typeid(val).name() + '\n');
  }

  // overrides for special cases
  template <>
  bool stringToType<bool>(const std::string str) {
    std::string lstr = lowerString(str);
    if (lstr == "true" || lstr == "1" || lstr == "false" || lstr == "0") return lstr == "true" || lstr == "1";
    else throw std::invalid_argument("Couldn't convert string \"" + str + "\" to type bool\n");
  }
  template <>
  std::string stringToType<std::string>(const std::string str) {
    return str;
  }

  // predefinition for friending
  class ArgParser;

  // base class for arguments
  class Argument {
  public:
    Argument(const std::string shortName, const std::string longName, const std::string description) : shortName{"-" + shortName}, longName{"--" + longName}, description{description} {}
    virtual ~Argument() = default;

    bool isSet() { return isSet_; }
    bool isDefined() { return isDefined_; }

    const std::string shortName;
    const std::string longName;
    const std::string description;
  protected:
    friend class ArgParser;
    bool isSet_{false};   // true only if the value was set in the command, not if it was set with a default value
    bool isDefined_{false}; // true if the value was set in the command or with a default value
    virtual void parseArg(char *arg) = 0;
  };

  // ValueArgs can (but don't have to) have a default value, and can be set in the commmand
  // if they are set in the command, they must be accompannied by a value (even if they have a default value)
  template <class T>
  class ValueArg : public Argument {
  public:
    // ctor without default value
    ValueArg(const std::string shortName, const std::string longName, const std::string description) : Argument(shortName, longName, description) {}
    // ctor with default value
    ValueArg(const std::string shortName, const std::string longName, const std::string description, const T defaultValue) : Argument(shortName, longName, description) {
      data = defaultValue;
      isDefined_ = true;
    }
    T value() {
      return data;
    }
  private:
    friend class ArgParser;
    T data;
    void parseArg(char *arg) override {
      // error check for no parameter
      if (arg[0] == '-') {
        // ternary here to make error messages look pretty
        throw std::invalid_argument("Command-line argument " + shortName + (shortName != "" && longName != "" ? "/" : "") + longName + " requires a value but none was given\n");
      }

      try {
        data = stringToType<T>(arg);
      } catch (std::invalid_argument) { // rethrow the error with a more informative description
        throw std::invalid_argument("Command-line argument " + shortName + (shortName != "" && longName != "" ? "/" : "") + longName + " recieved an invalid value of \"" + arg + "\"\n");
      }
    }
  };

  // ImplicitArgs must have a default value, but are undefined unless they appear in the command
  // if they appear in the command and are given a value, they are equal to that value,
  // otherwise they are equal to their default value
  template <typename T>
  class ImplicitArg : public Argument {
  public:
    ImplicitArg(const std::string shortName, const std::string longName, const std::string description, const T dv) : Argument(shortName, longName, description) {
      defaultValue = dv;
    }
    T value() {
      return data;
    }
  private:
    friend class ArgParser;
    T data;
    T defaultValue;
    void parseArg(char *arg) override {
      // set data to the default value if no value is given in the command
      if (arg[0] == '-') {
        data = defaultValue;
        isDefined_ = true;
        return;
      }

      try {
        data = stringToType<T>(arg);
      } catch (std::invalid_argument) { // rethrow the error with a more informative description
        throw std::invalid_argument("Command-line argument " + shortName + (shortName != "" && longName != "" ? "/" : "") + longName + " recieved an invalid value of \"" + arg + "\"\n");
      }

      isDefined_ = true;
      isSet_ = true;
    }
  };

  // Flag arguments can only be booleans and cannot have a default value
  // if a flag argument appears in the command, it is equal to true,
  // otherwise, it is equal to false
  class FlagArg : public Argument {
  public:
    FlagArg(const std::string shortName, const std::string longName, const std::string description) : Argument(shortName, longName, description) {
      data = false;
      isDefined_ = true;
    }
    bool value() {
      return data;
    }
  private:
    friend class ArgParser;
    bool data;
    void parseArg(char *arg) override {
      data = true;
      isSet_ = true;
    }
  };

  class ArgParser {
  public:
    template <typename ArgType, typename... Args>
    auto add(Args&&... args) {
      static_assert(std::is_base_of<Argument, ArgType>::value, "Command-line arguments must be of type FlagArg, ImplicitArg, or ValueArg\n");
      ArgType arg{args...};
      auto argPtr = std::make_shared<ArgType>(arg);

      // ensure the argument has at least one name
      if (arg.shortName == "-" && arg.longName == "--") throw std::invalid_argument("Command-line arguments must have at least one name\n");
      if (arg.shortName != "-") {
        arguments[arg.shortName] = argPtr;
      }
      if (arg.longName != "--") {
        arguments[arg.longName] = argPtr;
      }
      return argPtr;
    }

    void parseCmd(int argc, char **argv) {
      char *next{};
      for (int i{1}; i < argc; ++i) { // start at i=1 because the first item in argv is always the path to the executable
        if (argv[i] == "") continue; // i don't actually know if it's possible for an empty string to end up in argv, but i'm also not going to risk it

        if (i < argc - 1) next = argv[i + 1];
        else next[0] =  '-'; //

        // check if the current item is the name of an argument, and call the argument's parseArg() if it is
        auto argIt = arguments.find(argv[i]);
        if (argIt != arguments.end()) {
          argIt->second->parseArg(next);
        }
      }
    }
  private:
    // arguments are stored as shared pointers so that they can have keys for both their long and short names
    std::unordered_map<std::string, std::shared_ptr<Argument>> arguments;
  };
}