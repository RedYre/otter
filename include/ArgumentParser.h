#ifndef ARGUMENTPARSER_H_
#define ARGUMENTPARSER_H_
#include "ArgumentOption.h"

#include <string>
#include <vector>
#include <iostream>

/*
 * used to set matched member of argument option which is vital for parsing args
 */
namespace Otter
{
  class ArgumentParser
  {
  public:
    ArgumentParser(std::vector<std::string> argv, std::vector<Otter::ArgumentOption> &argOptions);
  };
}
#endif // ARGUMENTPARSER_H_
