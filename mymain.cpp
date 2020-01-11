//
// Created by t123yh on 2020/1/3.
//
#include <fstream>
#include <iostream>
#include <Font.h>
#include <yaml-cpp/yaml.h>
#include <map>
#include "json.hpp"
#include "inja.hpp"
#include "mcuprintf.h"
#include "freetype_import.hh"
#include "optimize_rlefont.hh"
#include "export_rlefont.hh"
#include "exporttools.hh"

using namespace mcufont;

// Just for convenience
using namespace inja;
using json = nlohmann::json;

std::string cstring(const std::string& original)
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

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: encoder <config.yaml>";
        return 1;
    }
    std::map<int, Language> languages;
    std::map<std::string, ConfigFont> fonts;
    std::vector<UIString> uiStrings;
    
    YAML::Node config = YAML::LoadFile(argv[1]);
    std::string outputDir = config["OutputDirectory"].as<std::string>() + "/";
    std::string stringsTemplate = config["StringsTemplate"].as<std::string>();
    int optimizeIterations = config["OptimizeIterations"].as<int>();
    for (auto node : config["Fonts"])
    {
        fonts.insert({node["Name"].as<std::string>(),
                      ConfigFont(mcufont::filename_to_identifier(node["Name"].as<std::string>()),
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
        UIString obj{name};
        
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
                    ConfigFont &font = fonts.at(lang_content["Font"].as<std::string>());
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
        std::ofstream fontsHeader(outputDir + "fonts.h");
        std::ofstream fontsSource(outputDir + "fonts.cpp");
        fontsHeader << "#include <rlefont_def.h>" << std::endl;
        fontsSource << "#include <rlefont_def.h>" << std::endl;
        for (auto &cfgFont : fonts)
        {
            std::cout << "Processing " << cfgFont.second.Name << " (" << cfgFont.second.Characters.size()
                      << " characters)" << std::endl;
            cfgFont.second.Characters.insert(' ');
            cfgFont.second.Data = LoadFreetype(cfgFont.second.TTFPath, cfgFont.second.Size, cfgFont.second.Characters);
            mcufont::rlefont::init_dictionary(*cfgFont.second.Data);
            
            size_t oldsize = mcufont::rlefont::get_encoded_size(*cfgFont.second.Data);
            std::cout << "Original size is " << oldsize << " bytes" << std::endl;
            
            int i = 0, limit = optimizeIterations;
            time_t oldtime = time(NULL);
            while (i < limit)
            {
                mcufont::rlefont::optimize(*cfgFont.second.Data);
                
                size_t newsize = mcufont::rlefont::get_encoded_size(*cfgFont.second.Data);
                time_t newtime = time(NULL);
                
                int bytes_per_min = (oldsize - newsize) * 60 / (newtime - oldtime + 1);
                
                i++;
                std::cout << "iteration " << i << ", size " << newsize
                          << " bytes, speed " << bytes_per_min << " B/min"
                          << std::endl;
                if (newsize < 20)
                    break;
            }
            
            std::string def = mcufont::rlefont::write_source(fontsSource, cfgFont.second.Name, *cfgFont.second.Data);
            std::cout << "Wrote " << cfgFont.second.Name << std::endl;
            fontsHeader << def << std::endl;
        }
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
            json obj{{"name", str.Name}};
            if (str.Default)
            {
                obj["value"] = cstring(str.Default->Value);
                obj["font"] = str.Default->Font->Name;
            }
            else
            {
                auto& cont = obj["languages"] = {};
                for (const auto &lang : str.Langs)
                {
                    cont.push_back(json{{"lang", languages[lang.first].Name}, {"value", cstring(lang.second.Value)}, {"font", lang.second.Font->Name}});
                }
            }
            strings.push_back(obj);
        }
        std::ifstream t(stringsTemplate);
        std::stringstream buffer;
        buffer << t.rdbuf();
        std::ofstream stringsFile(outputDir + "strings.h");
        render_to(stringsFile, buffer.str(), dat);
    }
}
