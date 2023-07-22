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