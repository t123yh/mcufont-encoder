//
// Created by t123yh on 2020/1/9.
//

#ifndef MFENCODER_MCUPRINTF_H
#define MFENCODER_MCUPRINTF_H
#include <string>
#include <set>
#include "encoding.h"

std::set<mf_char> str2set(const char *format);
std::string mcuprintf(const char *format);

#endif //MFENCODER_MCUPRINTF_H
