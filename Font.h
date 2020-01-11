//
// Created by t123yh on 2020/1/3.
//

#ifndef MFENCODER_FONT_H
#define MFENCODER_FONT_H

#include <vector>
#include <string>
#include <set>
#include <map>
#include <optional>
#include <memory>
#include "encoding.h"
#include "datafile.hh"

struct Language {
    std::string Name;
    int Id;
};

struct ConfigFont
{
    std::string Name;
    std::string TTFPath;
    int Size;
    std::set<mf_char> Characters;
    std::unique_ptr<mcufont::DataFile> Data;
    
    ConfigFont(std::string name, std::string ttfPath, int size) : Name(name), TTFPath(ttfPath), Size(size), Characters(), Data(nullptr)
    {
    
    }
};

struct StringItem
{
    std::string Value;
    const ConfigFont* Font;
};

struct UIString
{
    std::string Name;
    std::optional<StringItem> Default;
    std::map<int, StringItem> Langs;
};

#endif //MFENCODER_FONT_H
