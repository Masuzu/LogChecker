#include "LogChecker.h"
#include "tinyxml2.h"

#include <iostream>
#include <regex.h>
#include <string.h>

#include <sstream>

std::vector<std::string> &Split(const std::string &s, char delim, std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}


std::vector<std::string> Split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  Split(s, delim, elems);
  return elems;
}

Category::~Category()
{
  for(std::set<RegexPattern>::const_iterator it = regex_patterns_.begin(); it != regex_patterns_.end(); ++it)
    regfree((*it).regex);
}

int Category::operator<(const Category &cat) const
{
  return strcmp(name_.c_str(), cat.name_.c_str());
}

LogChecker::~LogChecker()
{
  for(std::map<std::string, Category *>::const_iterator it = categories_.begin(); it != categories_.end(); ++it)
    delete it->second;
}

static void ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
  if(from.empty())
    return;
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
  }
}

static bool StartsWith(const std::string& s1, const std::string& s2) {
  return s2.size() <= s1.size() && s1.compare(0, s2.size(), s2) == 0;
}

static bool EndsWith(const std::string s1, const std::string &s2) {
  return s2.size() <= s1.size() && s1.compare(s1.size()-s2.size(), s2.size(), s2) == 0;
}

static std::string kIntPattern = "([-+]?[0-9]*)";
static std::string kUintPattern = "([0-9]*)";
static std::string kDoublePattern = "([-+]?[0-9]*\\.?[0-9]+([eE][-+]?[0-9]+)?)";
static std::string kStringPattern = "(.?*)";

void LogChecker::LoadFromXML(const std::string &file_name)
{
  tinyxml2::XMLDocument doc(true, tinyxml2::PRESERVE_WHITESPACE);
  doc.LoadFile(file_name.c_str());
  tinyxml2::XMLElement* root = doc.RootElement();
  for(const tinyxml2::XMLElement* element = root->FirstChildElement(); element; element = element->NextSiblingElement())
  {
    std::string category_name = element->Attribute("name");
    std::cout << "Category name: " << category_name << std::endl;
    Category *category = new Category(category_name);
    categories_.insert(std::make_pair(category_name, category));
    for(const tinyxml2::XMLElement* filter = element->FirstChildElement(); filter; filter = filter->NextSiblingElement())
    {
      const char *match_string = filter->GetText();
      if(strcmp(filter->Name(), "match") == 0)
      {
        std::cout << "    " << category_name << ".match=" << match_string << std::endl;
        category->AddMatchedName(match_string);
      }
      else if(strcmp(filter->Name(), "pattern") == 0)
      {
        std::string pattern = match_string;
        std::cout << "    " << category_name << ".pattern=" << pattern.c_str() << std::endl;
        //ReplaceAll(pattern, ".", "\\.");
        std::string built_pattern;

        std::set<int> value_group_idx;
        std::set<int> key_group_idx;
        int num_groups = 1;

        bool skip = false;
        while(!pattern.empty()) {
          int idx = pattern.find("{{");
          if(skip) {
            skip = false;
            built_pattern += ".?*";
            if(idx == 0) {
              std::cout << "A {{skip}} tag can't be followed directly by another tag\n";
              exit(0);
            }
          }

          // Parse the pattern, extract the tags and build the corresponding regex in 'built_pattern'
          if(idx != std::string::npos) {
            built_pattern += pattern.substr(0, idx);
            pattern = pattern.substr(idx+2);
            idx = pattern.find("}}");
            std::string tag = pattern.substr(0, idx);
            if(StartsWith(tag, "uint")) {
              built_pattern += kUintPattern;
              key_group_idx.insert(num_groups);
              ++num_groups;
            }
            else if(StartsWith(tag, "int")) {
              built_pattern += kIntPattern;
              if(!EndsWith(tag, "value"))
                key_group_idx.insert(num_groups);
              else
                value_group_idx.insert(num_groups);
              ++num_groups;
            }
            else if(StartsWith(tag, "double")) {
              built_pattern += kDoublePattern;
              if(!EndsWith(tag, "value"))
                key_group_idx.insert(num_groups);
              else
                value_group_idx.insert(num_groups);
              num_groups += 2;
            }
            else if(tag == "skip_until_int") {
              built_pattern += "[^[0-9]]*";
            }
            else if(tag == "skip")
              skip = true;
            else if(StartsWith(tag, "string")) {
              built_pattern += kStringPattern;
              if(!EndsWith(tag, "value"))
                key_group_idx.insert(num_groups);
              else
                value_group_idx.insert(num_groups);
              ++num_groups;
            }
            pattern = pattern.substr(idx+2);
          }
          else {
            built_pattern += pattern;
            break;
          }
        }

        category->AddRegexPattern(built_pattern.c_str(), num_groups, value_group_idx, key_group_idx);
      }
      else if(strcmp(filter->Name(), "regex") == 0)
      {
        std::cout << "    " << category_name << ".regex=" << match_string << std::endl;

        if(!filter->Attribute("num_groups"))
        {
          std::cout << "    Missing attribute num_groups for element match\n";
          continue;
        }
        std::set<int> value_group_idx;
        if(filter->Attribute("value_group_idx"))
        {
          std::vector<std::string> tokens = Split(filter->Attribute("value_group_idx"), ',');
          for(int i=0, len = tokens.size(); i<len; ++i)
            value_group_idx.insert(atoi(tokens[i].c_str()));
        }

        std::set<int> key_group_idx;
        if(filter->Attribute("key_group_idx"))
        {
          std::vector<std::string> tokens = Split(filter->Attribute("key_group_idx"), ',');
          for(int i=0, len = tokens.size(); i<len; ++i)
            key_group_idx.insert(atoi(tokens[i].c_str()));
        }
        category->AddRegexPattern(match_string, atoi(filter->Attribute("num_groups")), value_group_idx, key_group_idx);
      }
      else
        std::cout << "    Unknown child element " << filter->Name() << std::endl;
    }
  }
}

void Category::AddRegexPattern(const std::string &regex_pattern, size_t num_groups,
                               const std::set<int> &value_group_idx, const std::set<int> &key_group_idx)
{
  regex_t *regex_compiled = new regex_t;

  if (regcomp(regex_compiled, regex_pattern.c_str(), REG_EXTENDED))
  {
    printf("Could not compile regular expression %s.\n", regex_pattern.c_str());
  }
  RegexPattern pattern;
  pattern.key_group_idx = key_group_idx;
  pattern.pattern = regex_pattern;
  pattern.regex = regex_compiled;
  pattern.num_groups = num_groups;
  pattern.value_group_idx = value_group_idx;
  regex_patterns_.insert(pattern);
}

void Category::TryMatchByRegex(const std::string &source)
{
  bool found_match = false;
  for(std::set<RegexPattern>::const_iterator it = regex_patterns_.begin(); it != regex_patterns_.end(); ++it)
  {
    size_t num_groups = (*it).num_groups;
    regex_t *regex = (*it).regex;
    const char *cursor = source.c_str();
    regmatch_t groups[num_groups];
    if (regexec(regex, cursor, num_groups, groups, 0))
    {
      // No more matches
      continue;
    }
    ++num_matches_;
    found_match = true;

    // Retrieve the capturing groups for the regex pattern (*it)
    std::string key;
    std::string value;
    const std::set<int> &value_group_idx = (*it).value_group_idx;
    const std::set<int> &key_group_idx = (*it).key_group_idx;

    int num_keys_found = 0, num_values_found = 0;
    for (int group_idx = 0; group_idx < num_groups; ++group_idx)
    {
      char cursor_copy[strlen(cursor) + 1];
      strcpy(cursor_copy, cursor);
      cursor_copy[groups[group_idx].rm_eo] = 0;
      char *group_value = (cursor_copy + groups[group_idx].rm_so);

      if(key_group_idx.find(group_idx) != key_group_idx.end())
      {
        key += group_value;
        ++num_keys_found;
        // Insert separator
        if(num_keys_found < key_group_idx.size())
          key += "|";
      }
      if(value_group_idx.find(group_idx) != value_group_idx.end())
      {
        value += group_value;
        ++num_values_found;
        // Insert separator
        if(num_values_found < (value_group_idx.size()))
          value += "|";
      }
      //printf("Capturing group[%i]: %s\n", group_idx, group_value);
    }
    //printf("%s->%s\n", key.c_str(), value.c_str());

    // Append the groups found to matches_
    MatchMap::iterator it = matches_.find(key);
    if(it == matches_.end())
    {
      matches_.insert(std::make_pair(key, std::multiset<std::string>()));
      it = matches_.find(key);
    }
    it->second.insert(value);
  }
}

void Category::Compare(const Category *category)  const
{
  std::cout << "There are " << category->num_matches_ << " matches found in the file to compare versus "
      << num_matches_ << " in the reference file for the category " << name_ << std::endl;
  for(MatchMap::const_iterator it = category->matches_.begin(); it != category->matches_.end(); ++it)
  {
    std::string key = it->first;
    const ValueSet &values = it->second;
    MatchMap::const_iterator it_ref = matches_.find(key);
    if(it_ref == matches_.end())
    {
      std::cout << "The key " << key << " with value " << *values.begin() << " could not be found in the reference file\n";
      continue;
    }
    const ValueSet &ref_values = matches_.find(key)->second;
    for(ValueSet::iterator it_val = values.begin(); it_val != values.end(); ++it_val)
    {
      if(ref_values.find(*it_val) == ref_values.end())
        std::cout << "The value " << *it_val << " for the key " << key << " differs in the two input files\n";
    }
  }

  // Symetrically check the equality of values with 'category' entries serving as the reference
  for(MatchMap::const_iterator it_ref = matches_.begin(); it_ref != matches_.end(); ++it_ref)
  {
    std::string key = it_ref->first;
    const ValueSet &values_ref = it_ref->second;
    MatchMap::const_iterator it = category->matches_.find(key);
    if(it == category->matches_.end())
    {
      std::cout << "The key " << key << " with value " << *values_ref.begin() << " could not be found in the second file\n";
      continue;
    }
    const ValueSet &values = category->matches_.find(key)->second;
    for(ValueSet::iterator it_val = values_ref.begin(); it_val != values_ref.end(); ++it_val)
    {
      if(values.find(*it_val) == values.end())
        std::cout << "The value " << *it_val << " for the key " << key << " differs in the two input files\n";
    }
  }
}

void LogChecker::AddRegexPattern(const std::string cat_name, const std::string &regex_pattern, size_t num_groups,
                                 const std::set<int> &value_group_idx, const std::set<int> &key_group_idx)
{
  if(categories_.find(cat_name) == categories_.end())
    categories_.insert(std::make_pair(cat_name, new Category(cat_name)));
  categories_[cat_name]->AddRegexPattern(regex_pattern, num_groups, value_group_idx, key_group_idx);
}

void LogChecker::TryMatchByRegex(const std::string &source)
{
  for(std::map<std::string, Category*>::iterator it = categories_.begin(); it != categories_.end(); ++it)
    it->second->TryMatchByRegex(source);
}

void LogChecker::Compare(const LogChecker &checker)
{
  for(std::map<std::string, Category*>::const_iterator it = categories_.begin(); it != categories_.end(); ++it)
  {
    std::map<std::string, Category*>::const_iterator it2 = checker.categories_.find(it->first);
    it->second->Compare(it2->second);
  }
}
