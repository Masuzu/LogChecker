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

void LogChecker::LoadFromXML(const std::string &file_name)
{
  tinyxml2::XMLDocument doc(true, tinyxml2::COLLAPSE_WHITESPACE);
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
      else if(strcmp(filter->Name(), "regex") == 0)
      {
        const char *match_string = filter->GetText();
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
  for(std::set<RegexPattern>::const_iterator it = regex_patterns_.begin(); it != regex_patterns_.end(); ++it)
  {
    size_t num_groups = (*it).num_groups;
    regex_t *regex = (*it).regex;
    const std::set<int> &value_group_idx = (*it).value_group_idx;
    const std::set<int> &key_group_idx = (*it).key_group_idx;
    const char *cursor = source.c_str();
    regmatch_t groups[num_groups];
    if (regexec(regex, cursor, num_groups, groups, 0))
    {
      // No more matches
      continue;
    }

    // Retrieve the capturing groups for the regex pattern (*it)
    std::string key;
    std::string value;

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
    printf("%s->%s\n", key.c_str(), value.c_str());
    // Append the groups found to matches_
    matches_.insert(std::make_pair(key, value));
  }
}

void Category::Compare(const Category *category)  const
{
  std::cout << "There are " << category->matches_.size() << " matches found in the file to compare versus "
      << matches_.size() << " in the reference file for the category " << name_ << std::endl;
  for(std::map<std::string, std::string>::const_iterator it = category->matches_.begin(); it != category->matches_.end(); ++it)
  {
    std::string key = it->first;
    std::string value = it->second;
    std::map<std::string, std::string>::const_iterator it_ref = matches_.find(key);
    if(it_ref == matches_.end())
    {
      std::cout << "The key " << key << " could not be found in the reference file\n";
      continue;
    }
    std::string ref_value = matches_.find(key)->second;
    if(value != ref_value)
      std::cout << "The value for the key " << key << " differs in the two input files\n";
  }

  // Symetrically check the equality of values with 'category' entries serving as the reference
  for(std::map<std::string, std::string>::const_iterator it_ref = matches_.begin(); it_ref != matches_.end(); ++it_ref)
  {
    std::string key = it_ref->first;
    std::string ref_value = it_ref->second;
    std::map<std::string, std::string>::const_iterator it = category->matches_.find(key);
    if(it == category->matches_.end())
    {
      std::cout << "The key " << key << " could not be found in the second file\n";
      continue;
    }
    std::string value = category->matches_.find(key)->second;
    if(value != ref_value)
      std::cout << "The value for the key " << key << " differs in the two input files\n";
  }
}

void LogChecker::AddRegexPattern(const std::string cat_name, const std::string &regex_pattern, size_t num_groups,
                                 const std::set<int> &capturing_group_idx, const std::set<int> &key_group_idx)
{
  if(categories_.find(cat_name) == categories_.end())
    categories_.insert(std::make_pair(cat_name, new Category(cat_name)));
  categories_[cat_name]->AddRegexPattern(regex_pattern, num_groups, capturing_group_idx, key_group_idx);
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
