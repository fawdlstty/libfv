cmake_minimum_required (VERSION 3.8)

project ("libfv")

include_directories (include)
file (GLOB HEADERS include/fv/*.h include/fv/*.hpp)
add_library(fv ${HEADERS})
