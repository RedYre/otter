#include "ArgumentParser.h"

Otter::ArgumentParser::ArgumentParser(std::vector<std::string> argv, std::vector<Otter::ArgumentOption> &argOptions)
{
  std::vector<bool> matched;
  for (std::string arg : argv)
  {
    matched.push_back(false);
  }
  // try to match argOptions with argv
  for (unsigned int i = 0; i < argv.size(); i += 1)
  {
    // std::cout << "matched[i], i:" << matched[i] << ","<< i << "\n";
    if (matched[i])
    {
      continue;
    }
    for (unsigned int j = 0; j < argOptions.size(); j += 1)
    {
      if (argv[i] == "-" + argOptions[j].getoption() ||
          argv[i] == "--" + argOptions[j].getoption())
      {
        // std::cout << "matched:" << argv[i] << "\n";
        matched[i] = true;
        argOptions[j].setmatched(true);
      }
    }
  }

  // recognize the parameter values
  for (unsigned int i = 0; i < argv.size(); i += 1)
  {
    // std::cout << "matched[i], i:" << matched[i] << ","<< i << "\n";

    for (unsigned int j = 0; j < argOptions.size(); j += 1)
    {
      if (argv[i] == "-" + argOptions[j].getoption() ||
          argv[i] == "--" + argOptions[j].getoption())
      {
        if (argOptions[j].getvals() + i < argv.size())
        {
          for (unsigned int k = 0; k < argOptions[j].getvals(); k += 1)
          {
            if (matched[i + k + 1])
            {
              std::cerr << "Problem while parsing args. Missing parameter for " << argv[i] << ".\n";
              exit(1);
            }
            if (argv[i + k + 1][0] != '-')
            {
              if (argOptions[j].gettype() == Otter::ArgumentOption::Type::STRING)
              {
                argOptions[j].addparamstring(argv[i + k + 1]);
                matched[i + k + 1] = true;
              }
              if (argOptions[j].gettype() == Otter::ArgumentOption::Type::INT)
              {
                argOptions[j].addparamint(std::stoi(argv[i + k + 1]));
                matched[i + k + 1] = true;
              }
            }
            else
            {
              std::cerr << "Problem while parsing args. \n";
              exit(1);
            }
          }
        }
        else
        {
          std::cerr << "Problem while parsing args. Is an argument missing?\n";
          exit(1);
        }
      }
    }
  }

  bool nonMatched = false;
  // print unmatched options
  for (unsigned int i = 0; i < argv.size(); i += 1)
  {
    // std::cout << "3matched[i], i:" << matched[i] << ","<< i << "\n";

    if (!matched[i])
    {
      nonMatched = true;
      std::cout << "Could not match:" << argv[i] << "\n";
    }
  }
  if (nonMatched)
  {
    std::cerr << "Problem while parsing args. \n";
    exit(1);
  }
}
