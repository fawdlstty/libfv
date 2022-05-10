# CMakeList.txt: libfv 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 将源代码添加到此项目的可执行文件。
add_executable (libfv_test "libfv.cpp")
set_property (TARGET libfv_test PROPERTY CXX_STANDARD 20)
include_directories ("../include")

# TODO: 如有需要，请添加测试并安装目标。

#find_package (Boost REQUIRED [COMPONENTS <libs>...])
#target_link_libraries (libfv_test PRIVATE Boost::boost Boost::<lib1> Boost::<lib2> ...)
find_package (asio CONFIG REQUIRED)
target_link_libraries (libfv_test PRIVATE asio asio::asio)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(libfv_test PRIVATE fmt::fmt)

find_path (GZIP_HPP_INCLUDE_DIRS "gzip/compress.hpp")
target_include_directories (libfv_test PRIVATE ${GZIP_HPP_INCLUDE_DIRS})

find_package (nlohmann_json CONFIG REQUIRED)
target_link_libraries (libfv_test PRIVATE nlohmann_json::nlohmann_json)

find_package (OpenSSL REQUIRED)
target_link_libraries (libfv_test PRIVATE OpenSSL::SSL OpenSSL::Crypto)

find_package (ZLIB REQUIRED)
target_link_libraries (libfv_test PRIVATE ZLIB::ZLIB)
