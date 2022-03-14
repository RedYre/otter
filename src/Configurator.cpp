#include "Configurator.h"

Otter::Configurator::Configurator(int argc, char *argv[])
{
  // assign the values of the argOptions vector to the
  // public member vars s.t. they can be referenced as public
  datainpathK = argOptions[0].getoption();

  dataoutpathK = argOptions[1].getoption();
  helpArg = argOptions[2].getoption();

  // assign the values of the rdfterms vector to the
  // public member vars s.t. they can be referenced as public

  // vector with only program suffix
  std::vector<std::string> arguments(argv + 1, argv + argc);
  if (arguments.size() > 0)
  {
    std::cout << "\nCommand line options were recognized.\n";
    Otter::ArgumentParser ap(arguments, argOptions);
    // first check if help arg was used
    if (containsArg(helpArg))
    {
      std::string fullDesc = "";
      for (unsigned int i = 0; i < argOptions.size(); i += 1)
      {
        fullDesc = fullDesc + "\"" + argOptions[i].getoption() +
                   "\"\t\t\"" + argOptions[i].getdescr() + "\"\n";
      }
      std::cout << fullDesc << "\n";
      exit(0);
    }
    // listen to the parsed args and get their values
    if (containsArg(datainpathK))
    {
      if (getArg(datainpathK).getparamstring().size() == 1)
      {
        datainpathV = getArg(datainpathK).getparamstring()[0];
      }
      else
      {
        std::cerr << "Could not find the corresponding value of datainpathK.\n";
        exit(1);
      }
    }
    else
    {
      std::cerr << "Could not match the input file\n";
      exit(1);
    }
    if (containsArg(dataoutpathK))
    {
      if (getArg(dataoutpathK).getparamstring().size() == 1)
      {
        dataoutpathV = getArg(dataoutpathK).getparamstring()[0];
      }
      else
      {
        std::cerr << "Could not find the corresponding value of datainpathK.\n";
        exit(1);
      }
    }
    else
    {
      std::cerr << "Could not match the output file\n";
      exit(1);
    }
  }
  else
  {
    std::cout << "\nNo command line options were recognized. "
                 "Abort.\n";
  }

  // check if input file path does exist
  if (!fileExists(datainpathV))
  {
    std::cerr << "Cannot find the input file.\n";
    exit(1);
  }

  // check if the folder of the output file exists
  if (dataoutpathV.find("/") != std::string::npos)
  {
    if (!fileExists(dataoutpathV.substr(0, dataoutpathV.find_last_of("/"))) && (dataoutpathV.substr(0, dataoutpathV.find_last_of("/"))) != "")
    {
      std::cerr << "Cannot find the output folder.\n";
      exit(1);
    }
  }
  else
  {
    std::cerr << "Cannot output file path that was given.";
    exit(1);
  }
}

Otter::Configurator::Configurator()
{
}

Otter::Configurator &Otter::Configurator::operator=(Otter::Configurator other)
{
  datainpathK = other.datainpathK;
  dataoutpathK = other.dataoutpathK;
  helpArg = other.helpArg;
  datainpathV = other.datainpathV;
  dataoutpathV = other.dataoutpathV;

  return *this;
}

bool Otter::Configurator::fileExists(std::string pfile)
{
  struct stat buffer;
  return (stat(pfile.c_str(), &buffer) == 0);
}

bool Otter::Configurator::containsArg(std::string name)
{
  for (unsigned int i = 0; i < argOptions.size(); i += 1)
  {
    if (argOptions[i].getoption() == name && argOptions[i].getmatched())
    {
      return true;
    }
  }
  return false;
}

Otter::ArgumentOption Otter::Configurator::getArg(std::string name)
{
  Otter::ArgumentOption ao("", "");
  for (unsigned int i = 0; i < argOptions.size(); i += 1)
  {
    if (argOptions[i].getoption() == name && argOptions[i].getmatched())
    {
      return argOptions[i];
    }
  }
  return ao;
}
