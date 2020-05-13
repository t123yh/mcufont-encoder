//
// Created by t123yh on 2020/1/3.
//
#include <fstream>
#include <filesystem>
#include <iostream>
#include <Font.h>
#include <yaml-cpp/yaml.h>
#include <map>
#include <set>
#include "json.hpp"
#include "inja.hpp"
#include "mcuprintf.h"
#include "freetype_import.hh"
#include "optimize_rlefont.hh"
#include "export_rlefont.hh"
#include "exporttools.hh"
#include "export_strings.h"

using namespace mcufont;

// Just for convenience
using namespace inja;
using json = nlohmann::json;

std::string cstring(const std::string &original)
{
    std::stringstream buffer;
    buffer << "\"";
    for (char ch : original)
    {
        if (ch == '"')
        {
            buffer << "\\\"";
        }
        else if (ch == '\\')
        {
            buffer << "\\\\";
        }
        else
        {
            buffer << ch;
        }
    }
    buffer << "\"";
    return buffer.str();
}

namespace fs = std::filesystem;

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        std::cout << "Usage: encoder <config.yaml> <Output Directory>";
        return 1;
    }
    std::map<int, Language> languages;
    std::map<std::string, ConfigFont> fonts;
    std::vector<UIString> uiStrings;
    
    fs::path configPath = fs::absolute(argv[1]), outputPath = fs::absolute(argv[2]), configDir = configPath.parent_path();
    YAML::Node config = YAML::LoadFile(configPath.string());
    std::string stringsTemplate = configDir / config["StringsTemplate"].as<std::string>();
    int optimizeIterations = config["OptimizeIterations"].as<int>();
    uint8_t fontid = 0;
    for (auto node : config["Fonts"])
    {
        fonts.insert({node["Name"].as<std::string>(),
                      ConfigFont(fontid++, mcufont::filename_to_identifier(node["Name"].as<std::string>()),
                                 node["TTF"].as<std::string>(),
                                 node["Size"].as<int>())});
    }
    int lang_i = 0;
    for (const auto &node : config["Languages"])
    {
        auto name = node.as<std::string>();
        languages.insert({lang_i, Language{name, lang_i++}});
    }
    
    for (auto node : config["Strings"])
    {
        auto name = node["Name"].as<std::string>();
        bool middle = false, erase = false;
        if (auto eraseNode = node["Erase"]; eraseNode)
        {
            erase = eraseNode.as<bool>();
        }
        if (auto middleNode = node["Middle"]; middleNode)
        {
            middle = middleNode.as<bool>();
        }
        UIString obj{name, middle, erase};
        
        if (auto all = node["Content-all"]; all)
        {
            ConfigFont &font = fonts.at(all["Font"].as<std::string>());
            auto str = all["Value"].as<std::string>();
            auto chset = str2set(str.c_str());
            font.Characters.merge(chset);
            obj.Default = StringItem{std::move(str), &font};
        }
        else
        {
            obj.Default = std::nullopt;
            for (auto &lang : languages)
            {
                if (auto lang_content = node["Content-" + lang.second.Name]; lang_content)
                {
                    std::string fname = lang_content["Font"].as<std::string>();
                    ConfigFont &font = fonts.at(fname);
                    auto str = lang_content["Value"].as<std::string>();
                    auto chset = str2set(str.c_str());
                    font.Characters.merge(chset);
                    obj.Langs.insert({lang.first, {std::move(str), &font}});
                }
            }
        }
        uiStrings.push_back(std::move(obj));
    }
    
    {
        std::ofstream ofs(outputPath / "strings.bin");
        export_strings(uiStrings, ofs);
    }
    {
        json dat;
        auto &langs = dat["languages"] = {};
        for (const auto &lang : languages)
        {
            langs.push_back(lang.second.Name);
        }
        
        auto &strings = dat["strings"] = {};
        for (const auto &str: uiStrings)
        {
            json obj{{"name", str.Name}, {"pos", str.Pos}};
            strings.push_back(obj);
        }
        auto& jfonts = dat["fonts"] = {};
        // Declaring the type of Predicate that accepts 2 pairs and return a bool
        typedef std::function<bool(const ConfigFont*, const ConfigFont*)> Comparator;
    
        // Defining a lambda function to compare two pairs. It will compare two pairs using second field
        Comparator compFunctor =
                [](const ConfigFont* elem1, const ConfigFont* elem2)
                {
                    return elem1->Id < elem2->Id;
                };
        
        std::vector<const ConfigFont*> fontList;
        for(auto& font: fonts)
        {
            fontList.push_back(&font.second);
        }
        
        sort(fontList.begin(), fontList.end(), compFunctor);
    
        for (const auto &font: fontList)
        {
            json obj{{"id", font->Id}, {"name", font->Name}};
            jfonts.push_back(obj);
        }
        
        std::ifstream t(stringsTemplate);
        std::stringstream buffer;
        buffer << t.rdbuf();
        std::ofstream stringsFile(outputPath / "strings.h");
        render_to(stringsFile, buffer.str(), dat);
    }
    
    uint32_t totalSize = 0;
    {
        std::ofstream fontsHeader(outputPath / "fonts.h");
        std::ofstream fontsSource(outputPath / "fonts.cpp");
        fontsHeader << "#include <rlefont_def.h>" << std::endl;
        fontsSource << "#include <rlefont_def.h>" << std::endl;
        for (auto &cfgFont : fonts)
        {
            std::cout << "Processing " << cfgFont.second.Name << " (" << cfgFont.second.Characters.size()
                      << " characters, ";
            cfgFont.second.Characters.insert(' ');
            cfgFont.second.Data = LoadFreetype((configDir / cfgFont.second.TTFPath).string(), cfgFont.second.Size, cfgFont.second.Characters);
            mcufont::rlefont::init_dictionary(*cfgFont.second.Data);
            
            size_t oldsize = mcufont::rlefont::get_encoded_size(*cfgFont.second.Data);
            std::cout << "original size is " << oldsize << " bytes)" << std::endl;
            
            size_t newsize;
            int i = 0, limit = optimizeIterations;
            time_t oldtime = time(NULL);
            while (i < limit)
            {
                mcufont::rlefont::optimize(*cfgFont.second.Data);
                
                newsize = mcufont::rlefont::get_encoded_size(*cfgFont.second.Data);
                time_t newtime = time(NULL);
                
                int bytes_per_min = (oldsize - newsize) * 60 / (newtime - oldtime + 1);
                
                i++;
                std::cout << "\33[2K\r" << "Iteration " << i << ", size " << newsize
                          << " bytes, speed " << bytes_per_min << " B/min";
                std::cout.flush();
                if (newsize < 20)
                    break;
            }
            std::cout << "\33[2K\rCompleted. Size of compressed data is " << newsize << " bytes." << std::endl;
            totalSize += newsize;
            
            std::string def = mcufont::rlefont::write_source(fontsSource, cfgFont.second.Name, *cfgFont.second.Data);
            fontsHeader << def << std::endl;
        }
        std::cout << "Total font size: " << totalSize << std::endl;
    }
    
}
