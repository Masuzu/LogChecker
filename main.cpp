#include "LogChecker.h"

#include <iostream>
#include <fstream>

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

//#define __TEST

int main(int argc, char *argv[])
{
#ifdef __TEST
  int num_groups = 13;
  regex_t *regex_compiled = new regex_t;
  char *regex_pattern = "(PAOMT : )([0-9]*)([^-]*- )([0-9]*)([^-]*- )([0-9]*)([^-]*- )([0-9]*)([^-]*- )(-?[0-9]*\\.?[0-9]*)([^-]*- )([0-9]*)";
  if (regcomp(regex_compiled, regex_pattern, REG_EXTENDED))
  {
    printf("Could not compile regular expression %s.\n", regex_pattern);
  }

  regmatch_t groups[num_groups];
  const char *cursor = "PAOMT : 0 - 1133 - 0 - 1 - 19804.2 - 0";
  //const char *cursor = "PAOMT : 0 - 1212 - 0 - 5 - -0 - 0";
  if (regexec(regex_compiled, cursor, num_groups, groups, 0))
  {
    // No more matches
    std::cout << "No matches found\n";
  }


  __StdSet(int, capturing_group_idx, 2, 4, 6, 8);
  for (int i = 0; i < num_groups; ++i)
  {
    char cursor_copy[strlen(cursor) + 1];
    strcpy(cursor_copy, cursor);
    cursor_copy[groups[i].rm_eo] = 0;
    char *group_value = (cursor_copy + groups[i].rm_so);

    printf("Capturing group[%i]: %s\n", i, group_value);
  }
#endif

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
