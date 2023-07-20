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
    Argument(const std::string &shortName, const std::string &longName, std::string description) :
          shortName{(!shortName.empty()) ? ("-" + shortName) : ("")}, longName{(!longName.empty()) ? ("--" + longName) : ""},
          description{std::move(description)} {}

   virtual ~Argument() = default;

    bool isSet() const { return isSet_; }
    bool isDefined() const { return isDefined_; }

    const std::string shortName;
    const std::string longName;
    const std::string description;
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
    // ctor without default value
    ValueArg(const std::string &shortName, const std::string &longName, const std::string &description) :
          Argument(shortName, longName, description) {}
    // ctor with default value
    ValueArg(const std::string &shortName, const std::string &longName, const std::string &description, const T &defaultValue) :
          Argument(shortName, longName, description) {
      data = defaultValue;
      isDefined_ = true;
      hasDefault = true;
    }
    T value() const {
      return data;
    }
  private:
    friend class ArgParser;
    T data;
    bool hasDefault{false};
    void parseArg(const char *arg) override {
      // error check for no parameter
      if (arg[0] == '-') {
        // ternary here to make error messages look pretty
        throw std::invalid_argument("Command-line argument " + shortName + (!shortName.empty() && !longName.empty() ? "/" : "")
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
      if (!hasDefault) return "";
      return {"=" + typeToString(data)};
    }
  };

  // ImplicitArgs must have a default value, but are undefined unless they appear in the command
  // if they appear in the command and are given a value, they are equal to that value,
  // otherwise they are equal to their default value
  template <typename T>
  class ImplicitArg : public Argument {
  public:
    ImplicitArg(const std::string &shortName, const std::string &longName, const std::string &description, const T &dv) :
          Argument(shortName, longName, description) {
      defaultValue = dv;
    }
    T value() const {
      return data;
    }
  private:
    friend class ArgParser;
    T data;
    T defaultValue;
    void parseArg(const char *arg) override {
      // set data to the default value if no value is given in the command
      if (arg[0] == '-') {
        data = defaultValue;
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
      return {"=arg(=" + typeToString(defaultValue) + ")"};
    }
  };

  // Flag arguments can only be booleans and cannot have a default value
  // if a flag argument appears in the command, it is equal to true,
  // otherwise, it is equal to false
  class FlagArg : public Argument {
  public:
    FlagArg(const std::string &shortName, const std::string &longName, const std::string &description) :
          Argument(shortName, longName, description) {
      isDefined_ = true;
    }
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
    std::string getDefaultAsString() override {
      return "";
    }
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
      if (arg.shortName != "") arguments[arg.shortName] = argPtr;
      if (arg.longName != "") arguments[arg.longName] = argPtr;
      return argPtr;
    }

    void parseCmd(int argc, const char **argv) {
      const char *next;
      for (int i{1}; i < argc; ++i) { // start at i=1 because the first item in argv is always the path to the executable
        // i don't actually know if it's possible for an empty string to end up in argv, but i'm also not going to risk it
        if (argv[i][0] == '\0' || argv[i] == nullptr) continue;

        if (i < argc - 1) next = argv[i + 1];
        else next = "";

        std::cout << next << '\n';

        // this would be auto, but "auto" is the only thing in this file that isn't supported by C++ 11
        auto argIt = arguments.find(argv[i]);
        if (argIt != arguments.end()) argIt->second->parseArg(next);
      }
    }

    // creates and returns a formatted help message containing every command
    std::string createHelpMessage() {
      // this isn't a great way to prevent duplicates,
      // but i tried storing pointers in a vectors and it didn't work for some reason
      std::vector<std::string> longNames;
      std::vector<std::string> shortNames;
      std::vector<std::string> defaultValues;
      std::vector<std::string> descriptions;
      // for formatting
      size_t longestShortName{0};
      size_t longestLongName{0};
      size_t longestDefaultValue{0};
      for (const auto& it : arguments) {
        auto arg{it.second};
        if (std::find(shortNames.begin(), shortNames.end(), arg->shortName) != shortNames.end()
            && std::find(longNames.begin(), longNames.end(), arg->longName) != longNames.end()) continue;

        shortNames.push_back(arg->shortName);
        if (arg->shortName.length() > longestShortName) longestShortName = arg->shortName.length();

        longNames.push_back(arg->longName);
        if (arg->longName.length() > longestLongName) longestLongName = arg->longName.length();

        defaultValues.push_back(arg->getDefaultAsString());
        if (arg->getDefaultAsString().length() > longestDefaultValue) longestDefaultValue = arg->getDefaultAsString().length();

        descriptions.push_back(arg->description);
      }

      std::ostringstream helpMessage;
      helpMessage << "[[Allowed Arguments]]\n";

      // shortNames and longNames will always be the same size
      for (size_t i{0}; i < shortNames.size(); ++i) {
        // the long name and the default value are merged together to make formatting work
        std::string longNameDefaultValue{longNames[i] + " " + defaultValues[i]};

        helpMessage << "  " << std::right << std::setw(static_cast<int>(longestShortName)) << shortNames[i] << std::setw(0) << ", "
                    << std::left << std::setw(static_cast<int>(longestLongName + longestDefaultValue + 1)) << longNameDefaultValue
                    << std::setw(0) << "  " << descriptions[i] << '\n';
      }

      return helpMessage.str();
    }
  private:
    // arguments are stored as shared pointers so that they can have keys for both their long and short names
    std::map<std::string, std::shared_ptr<Argument>> arguments;
  };
}

#endif
#pragma clang diagnostic pop