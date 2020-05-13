cmake_minimum_required(VERSION 3.13)
project(yamfencoder)

find_package(Freetype REQUIRED)
find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)

include_directories(${FREETYPE_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 17)

include_directories(.)

add_executable(yamfencoder
        ccfixes.hh
        datafile.cc
        datafile.hh
        encode_rlefont.cc
        mcuprintf.cpp
        export_rlefont.cc
        export_rlefont.hh
        exporttools.cc
        exporttools.hh
        freetype_import.cc
        freetype_import.hh
        importtools.cc
        importtools.hh
        main.cc
        optimize_rlefont.cc
        optimize_rlefont.hh encoding.cpp encoding.h mymain.cpp Font.cpp Font.h mcuprintf.h export_strings.cpp export_strings.h)

target_link_libraries(yamfencoder ${FREETYPE_LIBRARIES} ${YAML_CPP_LIBRARIES} Threads::Threads)

install(TARGETS yamfencoder
        RUNTIME DESTINATION bin)
