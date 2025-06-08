/**
*** Author: R-CO
*** E-mail: daniel1820kobe@gamil.com
*** Date: 2019-11-26
**/
#include <fstream>
#include <iostream>
#include <cstdlib>

#include <regex>
#include <string>
#include <vector>

#include "SimpleIni.h"

#define DEBUG_RCO 0

namespace rco
{

struct UnitAbilities
{
  std::string unit_ability_id; // X1
  std::vector<std::string> hero_ability_list; // X6

  void Clear() {
    unit_ability_id.clear();
    hero_ability_list.clear();
  }
};

class UnitAbilitiesSlkParser
{
public:
  bool ParseFile(const std::string& file_name);
  const std::vector<UnitAbilities>& GetUnitAbilitiesVector() const { return unit_abilities_vector_; }
private:
  bool ReadFile(const std::string& file_name, std::vector<std::string>* lines_of_file);
  void ParseLine(const std::string& line, UnitAbilities *unit_abilities);
  bool ParseX1(const std::string& line, UnitAbilities* unit_abilities);
  bool ParseX6(const std::string& line, UnitAbilities* unit_abilities);
  std::vector<UnitAbilities> unit_abilities_vector_;
};

} // end of namespace "rco"

struct SkillInfo
{
  std::string name;
  std::string research_uber_tip;

  void Clear() {
    name.clear();
    research_uber_tip.clear();
  }
};

struct HeroInfo
{
  std::string name;
  std::vector<SkillInfo> skill_info_vector;

  void Clear() {
    name.clear();
    skill_info_vector.clear();
  }
};

int main(int argc, char* argv[])
{
  rco::UnitAbilitiesSlkParser unit_abilities_slk_parser;
  if (!unit_abilities_slk_parser.ParseFile("test_data\\UnitAbilities.slk")) {
    return EXIT_FAILURE;
  }
  
  CSimpleIni campaign_unit_strings_ini;
  campaign_unit_strings_ini.SetUnicode();
  if (campaign_unit_strings_ini.LoadFile("test_data\\CampaignUnitStrings.txt") != 0) {
    return EXIT_FAILURE;
  }

  CSimpleIni campaign_ability_strings_ini;
  campaign_ability_strings_ini.SetUnicode();
  if (campaign_ability_strings_ini.LoadFile("test_data\\CampaignAbilityStrings.txt") != 0) {
    return EXIT_FAILURE;
  }

  std::ofstream out_file("hero_skill_list.txt", std::ios::out | std::ios::binary);
  out_file.write("\xEF\xBB\xBF", 3); // UTF-8 BOM file header

  HeroInfo hero_info;
  SkillInfo skill_info;
  const char* hero_name = nullptr;
  const auto& unit_abilities_vector = unit_abilities_slk_parser.GetUnitAbilitiesVector();
  for (const auto& it : unit_abilities_vector) {
    hero_info.Clear();
    hero_name = campaign_unit_strings_ini.GetValue(it.unit_ability_id.c_str(), "Name");
    if (hero_name == nullptr) {
      continue;
    }
    hero_info.name = hero_name;
    out_file.write("[", 1); 
    out_file.write(hero_info.name.c_str(), hero_info.name.length());
    out_file.write("]\r\n", 3);
    for (const auto& hero_ability_it : it.hero_ability_list) {
      skill_info.Clear();
      const char* skill_name = campaign_ability_strings_ini.GetValue(hero_ability_it.c_str(), "Name");
      if (skill_name == nullptr) {
        continue;
      }
      skill_info.name = skill_name;
      const char* skill_research_uber_tip = campaign_ability_strings_ini.GetValue(hero_ability_it.c_str(), "Researchubertip");
      skill_info.research_uber_tip = (skill_research_uber_tip == nullptr) ? "" : skill_research_uber_tip;
      hero_info.skill_info_vector.push_back(skill_info);
      out_file.write(skill_info.name.c_str(), skill_info.name.length());
      out_file.write(" = ", 3);
      out_file.write(skill_info.research_uber_tip.c_str(), skill_info.research_uber_tip.length());
      out_file.write("\r\n", 2);
    }
  }
  out_file.close();

  return EXIT_SUCCESS;
}

namespace rco
{

bool UnitAbilitiesSlkParser::ParseFile(const std::string& file_name)
{
  std::vector<std::string> lines_of_file;
  if (!ReadFile(file_name, &lines_of_file)) {
    return false;
  }

  UnitAbilities unit_abilities;
  for (const auto& it : lines_of_file) {
    ParseLine(it, &unit_abilities);
  }

  return true;
}

bool UnitAbilitiesSlkParser::ReadFile(const std::string& file_name, std::vector<std::string>* lines_of_file)
{
  std::ifstream in_file(file_name);

  if (!in_file.is_open()) {
    return false;
  }

  std::string line;
  while (!in_file.eof()) {
    std::getline(in_file, line);
    lines_of_file->push_back(line);
#if DEBUG_RCO
    std::cout << line << std::endl;
#endif
  }

  in_file.close();

  return true;
}

void UnitAbilitiesSlkParser::ParseLine(const std::string& line, UnitAbilities* unit_abilities)
{
  if (ParseX1(line, unit_abilities)) {
    return;
  }
  else if (ParseX6(line, unit_abilities)) {
    return;
  }
}

bool UnitAbilitiesSlkParser::ParseX1(const std::string& line, UnitAbilities* unit_abilities)
{
  static const std::regex x1_regex("C;Y\\d+;X1;K\"(\\S+)\""); // C;Y1;X1;K"unitAbilID"
  std::smatch x1_smatch;
  static const auto kMatchSize = 2;
  enum class EnumMatchItem
  {
    kWholeMatch = 0,
    kIdMatch = 1
  };

  if (!std::regex_match(line, x1_smatch, x1_regex) || (x1_smatch.size() != kMatchSize)) {
    return false;
  }

  unit_abilities->Clear();
  unit_abilities->unit_ability_id = x1_smatch[static_cast<size_t>(EnumMatchItem::kIdMatch)].str();

  return true;
}

bool UnitAbilitiesSlkParser::ParseX6(const std::string& line, UnitAbilities* unit_abilities)
{
  static const std::regex x6_regex("C;X6;K\"(\\S+)\""); // C;X6;K"A07Z,A01Z,A1G7,AC3P,A01U"
  std::smatch x6_smatch;
  static const auto kMatchSize = 2;
  enum class EnumMatchItem
  {
    kWholeMatch = 0,
    kHeroAbilListMatch = 1
  };

  if (!std::regex_match(line, x6_smatch, x6_regex) || (x6_smatch.size() != kMatchSize)) {
    return false;
  }

  std::string hero_abil_list(x6_smatch[static_cast<size_t>(EnumMatchItem::kHeroAbilListMatch)]);
  static const std::regex comma_regex(",");
  static const std::sregex_token_iterator end_itor;
  std::sregex_token_iterator hero_abil_token_itor(hero_abil_list.begin(), hero_abil_list.end(), comma_regex, -1);
  while (hero_abil_token_itor != end_itor) {
    unit_abilities->hero_ability_list.push_back(*hero_abil_token_itor++);
  }

  unit_abilities_vector_.push_back(*unit_abilities);

  return true;
}

} // end of namespace "rco"
