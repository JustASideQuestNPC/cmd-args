#include <iostream>
#include "cmd-args.hpp"
using namespace CmdArgs;

int main(int argc, char **argv) {
  ArgParser parser;

  auto val1 = parser.add<ValueArg<float>>("", "val1", "", 3.14f);
  auto val2 = parser.add<ValueArg<std::string>>("", "val2", "");
  auto imp1 = parser.add<ImplicitArg<int>>("", "imp1", "", 10);
  auto imp2 = parser.add<ImplicitArg<int>>("", "imp2", "", 20);
  auto flag1 = parser.add<FlagArg>("", "flag1", "");
  auto flag2 = parser.add<FlagArg>("", "flag2", "");
  parser.parseCmd(argc, argv);

  std::cout << "val1: " << val1->value() << '\n'
            << "val2: " << val2->value() << '\n'
            << "imp1: " << imp1->value() << '\n'
            << "imp2: " << imp2->value() << '\n'
            << "flag1: " << flag1->value() << '\n'
            << "flag2: " << flag2->value() << '\n';

  return 0;
}