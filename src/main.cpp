#include <iostream>
#include "cmd-args.hpp"
using namespace CmdArgs;

int main(int argc, const char **argv) {
  ArgParser parser;

  auto help = parser.add<FlagArg>("h", "help", "prints a help message");
  auto val1 = parser.add<ValueArg<float>>("v", "val1", "value argument 1", 3.14f);
  auto val2 = parser.add<ValueArg<std::string>>("", "val2", "value argument 2");
  auto imp1 = parser.add<ImplicitArg<int>>("i", "imp1", "implicit argument 1", 10);
  auto imp2 = parser.add<ImplicitArg<int>>("", "imp2", "implicit argument 2", 20);
  auto flag1 = parser.add<FlagArg>("f", "flag1", "flag argument 1");
  auto flag2 = parser.add<FlagArg>("", "flag2", "flag argument 2");

  parser.parseCmd(argc, argv);

  if (help->value()) {
    std::cout << parser.createHelpMessage();
    return 0;
  }

  std::cout << "val1: " << val1->value() << '\n'
            << "val2: " << val2->value() << '\n'
            << "imp1: " << imp1->value() << '\n'
            << "imp2: " << imp2->value() << '\n'
            << "flag1: " << flag1->value() << '\n'
            << "flag2: " << flag2->value() << '\n';

  return 0;
}