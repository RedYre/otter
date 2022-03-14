#ifndef ARGUMENTOPTION_H_
#define ARGUMENTOPTION_H_

#include <string>
#include <vector>
#include <iostream>
#include "gtest/gtest.h"

/*
 * used for parsing program arguments
 */
namespace Otter
{
  class ArgumentOption
  {
  public:
    // shall an argument be interpreted as a string or a number
    enum Type
    {
      STRING,
      INT
    };
    // setting members
    ArgumentOption(std::string opt, std::string desc)
    {
      option = opt;
      descr = desc;
    }
    // setting members
    ArgumentOption(std::string opt, std::string desc, int n, Type typeOpt)
    {
      option = opt;
      descr = desc;
      vals = n;
      type = typeOpt;
    }

    // getters and setters are found in the following...
    std::string getoption()
    {
      return option;
    }
    std::string getdescr()
    {
      return descr;
    }
    int getvals()
    {
      return vals;
    }
    Type gettype()
    {
      return type;
    }
    std::vector<std::string> getparamstring()
    {
      return paramstring;
    }
    std::vector<int> getparamint()
    {
      return paramint;
    }
    void addparamstring(std::string toadd)
    {
      paramstring.push_back(toadd);
    }
    void addparamint(int toadd)
    {
      paramint.push_back(toadd);
    }
    void setmatched(bool match)
    {
      matched = match;
    }
    bool getmatched()
    {
      return matched;
    }

  private:
    FRIEND_TEST(ArgumentOptionTest, getAndSet);
    std::string option;
    std::vector<std::string> paramstring;
    std::vector<int> paramint;
    std::string descr;
    int vals = 0;
    Type type;
    bool matched = false;
  };
}
#endif // ARGUMENTOPTION_H_