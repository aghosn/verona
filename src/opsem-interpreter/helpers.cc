#include "helpers.h"

#include <algorithm>
#include <iostream>
#include <set>
#include <stdlib.h>

using namespace std;

// TODO(aghosn):
//  * handle errors in a less shitty way.
//  * Correlate source position with parsed words.
namespace mlexer::helpers
{
  set<char> delimiters = {
    '.',
    ',',
    '(',
    ')',
    '=',
    ':',
    '{',
    '}',
    '|',
    '&',
  };

  bool isDelimiter(char c)
  {
    return delimiters.contains(c);
  }

  bool isCharacter(char c)
  {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
  }

  bool isDigit(char c)
  {
    return ('0' <= c && c <= '9');
  }

  // We authorize names such as 123 since we don't have numbers?
  bool isValidName(string& str)
  {
    // TODO fix this for unicode
    /*
    for(auto c: str) {
      if (!isCharacter(c) && !isDigit(c)) {
        return false;
      }
    }*/
    return true;
  }

  vector<pair<string, int>> split(string& str)
  {
    vector<pair<string, int>> result;
    if (str.size() == 0)
    {
      return result;
    }
    int front = 0, current = 0;
    while (front < str.size())
    {
      // Find first non white space.
      while (front < str.size() && isspace(str.at(front)))
      {
        front++;
      }

      // Got over to the end
      if (front >= str.size())
      {
        return result;
      }

      bool delimiter = isDelimiter(str.at(front));

      // Find end of current non-white space.
      current = front + 1;
      while (!delimiter && current < str.size() &&
             !(isspace(str.at(current)) || isDelimiter(str.at(current))))
      {
        current++;
      }
      auto sub = str.substr(front, current - front);

      if (!delimiter && !isValidName(sub))
      {
        std::cerr << "Invalid name: '" << sub << "'" << endl;
        std::cerr << "Full line: '" << str << "'" << endl;
        exit(EXIT_FAILURE);
      }
      result.push_back(pair<string, int>{sub, front});
      front = current;
    }
    return result;
  }

} // namespace mlexer::helpers
