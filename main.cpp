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
  LogChecker test;
  // Retrieve <keys> and <value> matching the pattern: resultat blahblahblah <key1 as string>: P <value1 as double> Q = <value2 as double>  S = <value3 as uint>  I = <value4 as double>
  __StdSet(int, value_group_idx, 2, 4, 6, 8);
  __StdSet(int, key_group_idx, 1);
  test.AddRegexPattern("Test", "resultat [^\\s]* ([^:]*):[^P]*P ([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)MW[^Q]*Q  =   ([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)MVAR[^S]*S  =   ([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)MVA[^I]*I  =   ([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)kA",
   9, value_group_idx, key_group_idx);
  cout << test.TryMatchByRegex("		-->  resultat départ TUILERIE:  P 1.38607MW		Q  =   0.217258MVAR		S  =   1.40299MVA 		I  =   0.0405009kA") << endl;

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
