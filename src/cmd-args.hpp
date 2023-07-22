/*
 *                          __
 *      _________ ___  ____/ /     ____ __________ ______
 *     / ___/ __ `__ \/ __  /_____/ __ `/ ___/ __ `/ ___/
 *    / /__/ / / / / / /_/ /_____/ /_/ / /  / /_/ /__  /
 *    \___/_/ /_/ /_/\__,_/      \__,_/_/   \__, /____/
 *                                         /____/
 *    version 1.1.0
 *    https://github.com/JustASideQuestNPC/cmd-args
 *
 *    Copyright (C) 2023 Joseph Williams
 *
 *    This software may be modified and distributed under the terms of
 *    the MIT license. See the LICENSE file for details.
 *
 *    THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef CMD_ARGS_HPP
#define CMD_ARGS_HPP

// normally i wouldn't ignore warnings like this, but templates and member functions mean that things the demo
// doesn't use will probably get used somewhere else, and my IDE keeps crying about that
#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include <sstream>
#include <iomanip>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include <algorithm>

namespace CmdArgs {
  std::string lowerString(const std::string& str) {
    std::string lowered{str};
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return lowered;
  }

  enum Visibility {
    VISIBLE,
    HIDDEN,
    INVISIBLE
  };

  template <typename T>
  T stringToType(const std::string &str) {
    std::istringstream convert{str};
    T val;
    if (convert >> val) return val;
    else throw std::invalid_argument("Couldn't convert string \"" + str + "\" to type T \n");
  }
  // overrides for special cases
  template <>
  bool stringToType<bool>(const std::string &str) {
    std::string lstr = lowerString(str);
    if (lstr == "true" || lstr == "1" || lstr == "false" || lstr == "0") return lstr == "true" || lstr == "1";
    else throw std::invalid_argument("Couldn't convert string \"" + str + "\" to type bool\n");
  }
  template <>
  std::string stringToType<std::string>(const std::string &str) {
    return str;
  }
  // this is by no means perfect, but it should cover all the basic types
  template <typename T>
  std::string typeToString(const T val) {
    return std::to_string(val);
  }
  template <>
  std::string typeToString(const bool val) {
    return (val ? "true" : "false");
  }
  template <>
  std::string typeToString(const std::string val) { // NOLINT(performance-unnecessary-value-param)
    return val; // NOLINT(performance-no-automatic-move)
  }

  // predefinition for friending
  class ArgParser;

  // base class for arguments
  class Argument {
  public:
    Argument(const Visibility visibility, const std::string &shortName, const std::string &longName, std::string description) :
        shortName{(!shortName.empty()) ? ("-" + shortName) : ("")}, longName{(!longName.empty()) ? ("--" + longName) : ""},
        visibility{visibility}, description{std::move(description)} {}
    Argument(const std::string &shortName, const std::string &longName, std::string description) :
        Argument(VISIBLE, shortName, longName, std::move(description)) {}

   virtual ~Argument() = default;

    bool isSet() const { return isSet_; }
    bool isDefined() const { return isDefined_; }

    const std::string shortName;
    const std::string longName;
    const std::string description;
    const Visibility visibility;

  protected:
    friend class ArgParser;
    bool isSet_{false};     // true only if the value was set in the command, not if it was set with a default value
    bool isDefined_{false}; // true if the value was set in the command or with a default value
    virtual void parseArg(const char *arg) = 0;
    virtual std::string getDefaultAsString() = 0; // used for printing a help message
  };

  // ValueArgs can (but don't have to) have a default value, and can be set in the commmand
  // if they are set in the command, they must be accompannied by a value (even if they have a default value)
  template <class T>
  class ValueArg : public Argument {
  public:
    // ctors
    ValueArg(const Visibility visibility, const std::string &shortName, const std::string &longName, const std::string &description) :
        Argument(visibility, shortName, longName, description) {}
    ValueArg(const Visibility visibility, const std::string &shortName, const std::string &longName, const std::string &description, const T &defaultValue) :
        Argument(visibility, shortName, longName, description) {
      data = defaultValue;
      dv = defaultValue;
      isDefined_ = true;
      hasDefault_ = true;
    }
    ValueArg(const std::string &shortName, const std::string &longName, const std::string &description) :
        ValueArg(VISIBLE, shortName, longName, description) {}
    ValueArg(const std::string &shortName, const std::string &longName, const std::string &description, const T &defaultValue) :
        ValueArg(VISIBLE, shortName, longName, description, defaultValue) {}

    T value() const {
      return data;
    }
    bool hasDefault() const {
      return hasDefault_;
    }
    // returns the argument's default value. produces undefined behavior if the argument has a default value
    T defaultValue() const {
      return dv;
    }
  private:
    friend class ArgParser;
    T data, dv;
    bool hasDefault_{false};
    void parseArg(const char *arg) override {
      // error check for no parameter
      if (arg[0] == '-') {
        // completely unnecessary ternary here to make error messages look a little prettier
        throw std::invalid_argument("Command-line argument " + shortName + (!shortName.empty() && !longName.empty() ? "," : "")
                                    + longName + " requires a value but none was given\n");
      }

      try {
        data = stringToType<T>(arg);
      } catch (std::invalid_argument& err) { // rethrow the error with a more informative description
        throw std::invalid_argument("Command-line argument " + shortName + (!shortName.empty() && !longName.empty() ? "/" : "")
                                    + longName + " recieved an invalid value of \"" + arg + "\"\n");
      }
    }

    // *attempts* to return the default value (if it exists) as a string.
    std::string getDefaultAsString() override {
      if (!hasDefault_) return "";
      return {"=" + typeToString(data)};
    }
  };

  // ImplicitArgs must have a default value, but are undefined unless they appear in the command
  // if they appear in the command and are given a value, they are equal to that value,
  // otherwise they are equal to their default value
  template <typename T>
  class ImplicitArg : public Argument {
  public:
    // ctor without default value
    ImplicitArg(const Visibility visibility, const std::string &shortName, const std::string &longName, const std::string &description, const T &sv) :
        Argument(visibility, shortName, longName, description) {
      setValue = sv;
    }
    // ctor with default value
    ImplicitArg(const Visibility visibility, const std::string &shortName, const std::string &longName, const std::string &description, const T &sv, const T &dv) :
        Argument(visibility, shortName, longName, description) {
      setValue = sv;
      data = dv;
      hasDefault_ = true;
      isDefined_ = true;
    }
    ImplicitArg(const std::string &shortName, const std::string &longName, const std::string &description, const T &sv) :
        ImplicitArg(VISIBLE, shortName, longName, description, sv) {}
    ImplicitArg(const std::string &shortName, const std::string &longName, const std::string &description, const T &sv, const T &dv) :
        ImplicitArg(VISIBLE, shortName, longName, description, sv, dv) {}

    T value() const {
      return data;
    }
    T defaultSetValue() const {
      return setValue;
    }
    bool hasDefault() const {
      return hasDefault_;
    }
    T defaultValue() const {
      return dv;
    }
  private:
    friend class ArgParser;
    T data, setValue, dv;
    bool hasDefault_{false};

    void parseArg(const char *arg) override {
      // set data to the default value if no value is given in the command
      if (arg[0] == '-') {
        data = setValue;
        isDefined_ = true;
        return;
      }

      try {
        data = stringToType<T>(arg);
      } catch (std::invalid_argument&) { // rethrow the error with a more informative description
        throw std::invalid_argument("Command-line argument " + shortName + ", " + longName + " recieved an invalid value of \"" + arg + "\"\n");
      }

      isDefined_ = true;
      isSet_ = true;
    }

    // *attempts* to return the default value as a string.
    std::string getDefaultAsString() override {
      return {"=arg(=" + typeToString(setValue) + ")"};
    }
  };

  // Flag arguments can only be booleans and cannot have a default value
  // if a flag argument appears in the command, it is equal to true,
  // otherwise, it is equal to false
  class FlagArg : public Argument {
  public:
    FlagArg(const Visibility visibility, const std::string &shortName, const std::string &longName, const std::string &description) :
        Argument(visibility, shortName, longName, description) {
      isDefined_ = true;
    }
    FlagArg(const std::string &shortName, const std::string &longName, const std::string &description) :
        FlagArg(VISIBLE, shortName, longName, description) { }

    bool value() const {
      return data;
    }
  private:
    friend class ArgParser;
    bool data{false};
    void parseArg(const char *arg) override {
      data = true;
      isSet_ = true;
    }
    // flags never have a default value, but this is still required for the help message
    std::string getDefaultAsString() override { return ""; }
  };

  class ArgParser {
  public:
    template <typename ArgType, typename... Args>
    std::shared_ptr<ArgType> add(Args&&... args) {
      static_assert(std::is_base_of<Argument, ArgType>::value, "Command-line arguments must be of type FlagArg, ImplicitArg, or ValueArg\n");
      ArgType arg{args...};
      auto argPtr = std::make_shared<ArgType>(arg);

      // ensure the argument has at least one name
      if (arg.shortName == "" && arg.longName == "") throw std::invalid_argument("Command-line arguments must have at least one name\n");

      // create one map entry for each of the argument's names
      if (arg.shortName != "") arguments[arg.shortName] = argPtr;
      if (arg.longName != "") arguments[arg.longName] = argPtr;

      // add the argument to one of the two vectors based on its visibility (invisible arguments aren't added to either)
      if (arg.visibility == VISIBLE) visibleArgs.push_back(argPtr);
      else if (arg.visibility == HIDDEN) hiddenArgs.push_back(argPtr);

      return argPtr;
    }

    void parseCmd(int argc, const char **argv) {
      const char *next;
      for (int i{1}; i < argc; ++i) { // start at i=1 because the first item in argv is always the path to the executable
        // i don't actually know if it's possible for an empty string to end up in argv, but i'm also not going to risk it
        if (argv[i][0] == '\0' || argv[i] == nullptr) continue;

        if (i < argc - 1) next = argv[i + 1];
        else next = "";

        auto argIt = arguments.find(argv[i]);
        if (argIt != arguments.end()) argIt->second->parseArg(next);
      }
    }

    // creates and returns a formatted help message containing every command
    std::string createHelpMessage(bool showHidden=false) {

      // find the longest names and the longest default value
      size_t longestShortName{0};
      size_t longestLongName{0};
      size_t longestDefaultValue{0};
      for (const auto& arg : visibleArgs) {
        if (arg->shortName.length() > longestShortName) longestShortName = arg->shortName.length();
        if (arg->longName.length() > longestLongName) longestLongName = arg->longName.length();
        if (arg->getDefaultAsString().length() > longestDefaultValue) longestDefaultValue = arg->getDefaultAsString().length();
      }

      // also check the longest names and default value with the hidden arguments if they're enabled
      if (showHidden) {
        for (const auto& arg : hiddenArgs) {
          if (arg->shortName.length() > longestShortName) longestShortName = arg->shortName.length();
          if (arg->longName.length() > longestLongName) longestLongName = arg->longName.length();
          if (arg->getDefaultAsString().length() > longestDefaultValue) longestDefaultValue = arg->getDefaultAsString().length();
        }
      }

      std::ostringstream helpMessage;
      helpMessage << "[[Allowed Arguments]]\n";

      // print the visible arguments
      for (const auto &arg : visibleArgs) {
        // the long name and the default value are merged together to make formatting work
        std::string longNameDefaultValue{arg->longName + " " + arg->getDefaultAsString()};

        helpMessage << "  " << std::right << std::setw(static_cast<int>(longestShortName)) << arg->shortName << std::setw(0) << ", "
                    << std::left << std::setw(static_cast<int>(longestLongName + longestDefaultValue + 1)) << longNameDefaultValue
                    << std::setw(0) << "  " << arg->description << '\n';
      }

      // print the hidden arguments if they're enabled
      if (showHidden) {
        helpMessage << "[[Hidden Arguments]]\n";
        for (const auto &arg : hiddenArgs) {
          // the long name and the default value are merged together to make formatting work
          std::string longNameDefaultValue{arg->longName + " " + arg->getDefaultAsString()};

          helpMessage << "  " << std::right << std::setw(static_cast<int>(longestShortName)) << arg->shortName << std::setw(0) << ", "
                      << std::left << std::setw(static_cast<int>(longestLongName + longestDefaultValue + 1)) << longNameDefaultValue
                      << std::setw(0) << "  " << arg->description << '\n';
        }
      }

      return helpMessage.str();
    }
  private:
    // arguments are stored as shared pointers so that they can have keys for both their long and short names
    std::map<std::string, std::shared_ptr<Argument>> arguments;

    // used to separate argument visibilities in help messages (and to print arguments in order)
    std::vector<std::shared_ptr<Argument>> visibleArgs;
    std::vector<std::shared_ptr<Argument>> hiddenArgs;
  };
}

#endif
#pragma clang diagnostic pop