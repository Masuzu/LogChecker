#include "LogChecker.h"

#include <iostream>
#include <fstream>
#include <locale>

using namespace std;

template <typename T, size_t N>
    T* begin(T(&arr)[N]) { return &arr[0]; }
template <typename T, size_t N>
    T* end(T(&arr)[N]) { return &arr[0]+N; }

// Workaround to simplify std::set initialization
// Usage __StdSet(int, foo, 1, 2, 3, 4, 5) will create a std::set foo of int initialzed with the array {1, 2, 3, 4, 5}
#define __StdSet(type, name, ...) \
type name##_array[] = {__VA_ARGS__}; \
                      std::set<type> name(begin(name##_array), end(name##_array));

int main(int argc, char *argv[])
{
  if(argc != 3)
  {
    cout << "Syntax: ./LogChecker <filename_to_check_1> <filename_to_check_2>\n";
    return 0;
  }
  // Tests
  // LogChecker test;
  // char * source = "PAOMT : 0 - 1133 - 0 - 1 - 19804.2 - 0";
  // char * source = "PAOMT : 1 - 271 - 0 - 2 - -2880 - 0";
  // Retrieve <keys> and <value> matching the pattern: PAOMT : <key1 as uint> - <key2 as uint> - <key3 as uint> - <key4 as uint> - <value as signed double> - <key4 as uint>
  //__StdSet(int, value_group_idx, 6);
  //__StdSet(int, key_group_idx, 2, 4, 8);
  //test.AddRegexPattern("Test", "(PAOMT : )([0-9]*)([^-]*- )([0-9]*)([^-]*- )([0-9]*)([^-]*- )([0-9]*)([^-]*- )(-?[0-9]*\\.?[0-9]*)([^-]*- )([0-9]*)",
  // 13, value_group_idx, key_group_idx);

  LogChecker test1;
  test1.LoadFromXML("config.xml");
  ifstream file(argv[1]);
  string line;
  while (std::getline(file, line))
    test1.TryMatchByRegex(line);
  file.close();

  LogChecker test2;
  test2.LoadFromXML("config.xml");
  file.open(argv[2]);
  while (std::getline(file, line))
    test2.TryMatchByRegex(line);
  file.close();

  test1.Compare(test2);
}
