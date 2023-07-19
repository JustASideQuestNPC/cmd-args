#pragma once
#include <sstream>
#include <map>
#include <any>
#include <memory>

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
  template <class T>
  class Argument {
  public:
    Argument(const std::string shortName, const std::string longName, const std::string description) : shortName{shortName}, longName{longName}, description{description} {}
    T getData() { return data; }
    bool isDefined() { return isDefined_; }
    bool wasSet() { return wasSet_; }
    bool fail() { return failbit; }
    bool noParameterError() { return noParameter; }
    bool invalidParameterError() { return invalidParameter; };

    const std::string shortName;
    const std::string longName;
    const std::string description;
  protected:
    friend class ArgParser;
    T data;
    bool isDefined_{false}; // this is true if the value was set in the command, *or* if it was set from a default value
    bool wasSet_{false};     // this is true only if the value was set in the command
    bool failbit{false};
    bool noParameter{false};
    bool invalidParameter{false};

    virtual void parseArg(const std::string arg);
  };

  // basic arguments are set to a given value if they are found in the command,
  // otherwise they are undefined.
  template <class T>
  class ValueArg : public Argument<T> {
  public:
    ValueArg(const std::string shortName, const std::string longName, const std::string description) : Argument<T>(shortName, longName, description) {}
  protected:
    friend class ArgParser;
    void parseArg(const std::string arg) override {
      if (arg[0] == '-' || arg == "") {
        this->failbit = true;
        this->noParameter = true;
        return;
      }

      try {
        this->data = stringToType<T>(arg);
      } catch (std::invalid_argument) {
        this->failbit = true;
        this->invalidParameter = true;
        return;
      }

      this->wasSet_ = true;
      this->isDefined_ = true;
    }
  };

  // defaulted argments are set to a given value if they are found in the command,
  // otherwise they are set to their default value.
  template <class T>
  class DefaultedArg : public Argument<T> {
  public:
    DefaultedArg(const std::string shortName, const std::string longName, const std::string description, const T defaultValue) : Argument<T>(shortName, longName, description) {
      this->data = defaultValue;
      this->isDefined_ = true;
    }
  protected:
    friend class ArgParser;
    void parseArg(const std::string arg) override {
      if (arg[0] == '-' || arg == "") return;

      try {
        this->data = stringToType<T>(arg);
      } catch (std::invalid_argument) {
        this->failbit = true;
        this->invalidParameter = true;
        return;
      }
      this->wasSet_ = true;
    }
  };

  // implicit arguments are set to a given value if they are found in the command,
  // set to a default value if they are in the command without a given value,
  // and undefined if they are not in the command at all.
  template <class T>
  class ImplicitArg : public Argument<T> {
  public:
    ImplicitArg(const std::string shortName, const std::string longName, const std::string description, const T dv) : Argument<T>(shortName, longName, description) {
      this->defaultValue = dv;
    }
  protected:
    friend class ArgParser;
    void parseArg(const std::string arg) override {
      if (arg[0] == '-' || arg == "") {
        this->data = this->defaultValue;
        this->isDefined_ = true;
        return;
      }
      try {
        this->data = stringToType<T>(arg);
      } catch (std::invalid_argument) {
        this->failbit = true;
        this->invalidParameter = true;
        return;
      }
      this->isDefined_ = true;
      this->wasSet_ = true;
    }
  private:
    T defaultValue;
  };
  
  // flag arguments can only be booleans, and cannot be given a value.
  // if they are in the command they are true, otherwise, they are false
  class FlagArg : public Argument<bool> {
  public:
    FlagArg(const std::string shortName, const std::string longName, const std::string description) : Argument<bool>(shortName, longName, description) {
      this->data = false;
      this->isDefined_ = true;
    }
  protected:
    friend class ArgParser;
    void parseArg(const std::string arg) override {
      this->data = true;
      this->wasSet_ = true;
    }
  };

  class ArgParser {
  private:
    std::map<std::string, std::shared_ptr<Argument<std::any> > > argObjects;
  };
}