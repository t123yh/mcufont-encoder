// Function for importing any font supported by libfreetype.

#pragma once
#include <set>
#include "datafile.hh"
#include "encoding.h"

namespace mcufont {
std::unique_ptr<DataFile> LoadFreetype(const std::string &file, int size, const std::set<mf_char>& charset);
}
