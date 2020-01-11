// Write out the encoded data in C source code files for the mf_rlefont format.

#pragma once

#include "datafile.hh"
#include "encode_rlefont.hh"
#include <iostream>
#include <string>

namespace mcufont::rlefont {

std::string write_source(std::ostream &out, std::string name, const DataFile &datafile);

}

