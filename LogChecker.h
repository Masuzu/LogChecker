#ifndef LOGCHECKER_H
#define LOGCHECKER_H

#include "tinyxml2.h"
#include <string>
#include <set>
#include <map>
#include <multiset.h>
#include <vector>
#include <regex.h>

struct RegexPattern
{
  regex_t *regex;
  std::string pattern;
  size_t num_groups;
  // Indices of the captured groups defining the value of a found match
  // See LogChecker::AddRegexPattern
  std::set<int> value_group_idx;
  // Indices of the captured groups used to form the compound key after a match following 'pattern' is found
  // See LogChecker::AddRegexPattern
  std::set<int> key_group_idx;
  int operator<(const RegexPattern &_pattern)  const
  {
    return strcmp(pattern.c_str(), _pattern.pattern.c_str());
  }
};

class Category
{
private:
  std::string name_;
  std::set<std::string> matched_names_;
  std::set<RegexPattern> regex_patterns_;
  typedef std::multiset<std::string> ValueSet;
  typedef std::map<std::string, std::multiset<std::string> > MatchMap;
  // Multiple entries may have the same key
  MatchMap matches_;
  int num_matches_;

public:
  explicit Category(const std::string &name) : name_(name), num_matches_(0) {}
  virtual ~Category();
  void AddMatchedName(const std::string &name) {matched_names_.insert(name);}

  inline const std::set<std::string> &matched_names() const {return matched_names_;}
  inline const std::string &name()  const {return name_;}
  int operator<(const Category &cat) const;
  // See LogChecker::AddRegexPattern
  void AddRegexPattern(const std::string &regex_pattern, size_t num_groups,
                       const std::set<int> &value_group_idx, const std::set<int> &key_group_idx);
  // See LogChecker::AddPattern
  void AddPattern(const std::string &pattern);
  bool TryMatchByRegex(const std::string &source);
  void Compare(const Category *category)  const;
};

// Classify data in categories based on string matching
class LogChecker
{
private:
  // Map matched names to the corresponding category
  std::map<std::string, Category *> categories_;

  typedef std::vector<std::string> CapturedGroupList;

public:
  LogChecker() {}
  virtual ~LogChecker();

  inline void AddCategory(Category *cat) {categories_.insert(std::make_pair(cat->name(), cat));}
  void LoadFromXML(const std::string &file_name);

  // Each input to compare (passed as a string) is matched againt the categories registered for this LogChecker. A category is a collection of
  // regular expressions which will be matched against the input. The capturing groups of each regular expression define either keys or values that
  // characterize the input.
  // For instance, given an input with the format "id=XXX id2=YYY value=ZZZ" and the regex (id=)([^\s]*)( id2=)([^\s]*)( value=)([^\s]*),
  // the captured groups retrieved will be:
  // Capturing group[0]: id=XXX id2=YYY value=ZZZ
  // Capturing group[1]: id=
  // Capturing group[2]: XXX
  // Capturing group[3]:  id2=
  // Capturing group[4]: YYY
  // Capturing group[5]:  value=
  // Capturing group[6]: ZZZ
  // To let LogChecker know that XXX and YYY are the keys identifying the input and  that ZZZ is the associated value, we have to set
  // key_group_idx={2, 4} and value_group_idx={6}
  void AddRegexPattern(const std::string &cat_name, const std::string &regex_pattern, size_t num_groups,
                       const std::set<int> &value_group_idx, const std::set<int> &key_group_idx);
  void AddPattern(const std::string &cat_name, const std::string &pattern);
  bool TryMatchByRegex(const std::string &source);

  void Compare(const LogChecker &checker);
};

#endif // LOGCHECKER_H
