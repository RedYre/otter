#ifndef UTIL_H_
#define UTIL_H_

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <cctype>
#include <iomanip>
#include <sstream>
/*
 * this file contains helper functions for the projects
 */

// split string by delimiter into vector
inline std::vector<std::string> cppsplit(std::string splitme, std::string delimiter)
{
  std::vector<std::string> result;

  size_t pos = 0;
  std::string token;
  while ((pos = splitme.find(delimiter)) != std::string::npos)
  {
    token = splitme.substr(0, pos);
    result.push_back(token);
    splitme.erase(0, pos + delimiter.length());
  }
  result.push_back(splitme);
  return result;
}

// replace substrings within "original" string with "after" string
inline std::string replaceAll(std::string const &original,
                              std::string const &before,
                              std::string const &after)
{
  std::string retval;
  std::string::const_iterator end = original.end();
  std::string::const_iterator current = original.begin();
  std::string::const_iterator next =
      std::search(current, end, before.begin(), before.end());
  while (next != end)
  {
    retval.append(current, next);
    retval.append(after);
    current = next + before.size();
    next = std::search(current, end, before.begin(), before.end());
  }
  retval.append(current, next);
  return retval;
}

// used to encode certain characters in a literal string to ensure valid rdf output
inline void encodeLiteral(std::string &data)
{
  std::string buffer;
  buffer.reserve(data.size());
  for (size_t pos = 0; pos != data.size(); ++pos)
  {
    switch (data[pos])
    {
    case '\t':
      buffer.append("\\t");
      break;
    case '"':
      buffer.append("\\\"");
      break;
    case '\\':
      buffer.append("\\\\");
      break;
    case '\n':
      buffer.append("\\n");
      break;
    case '\r':
      buffer.append("\\r");
      break;
    default:
      buffer.append(&data[pos], 1);
      break;
    }
  }
  data.swap(buffer);
}

// used to encode certain characters to ensure valid rdf output
inline std::string url_encode(const std::string &value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
  {
    std::string::value_type c = (*i);

    // Keep alphanumeric and other accepted characters intact
    if (std::isalnum(c) || c == '-' || c == '_' /*|| c == '.' || c == '~' */)
    {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    escaped << std::uppercase;
    escaped << '%' << std::setw(2) << int((unsigned char)c);
    escaped << std::nouppercase;
  }

  return escaped.str();
}

// used to print terminal output in main/reader class
static void print(std::string messag)
{
  std::string otters = "[OTTER-LOG] ";
  std::cout << otters << messag << "\n";
}
static void print(std::string messag, long long int x)
{
  std::string otters = "[OTTER-LOG] ";
  std::cout << otters << messag << x << "\n";
}

#endif // UTIL_H_
