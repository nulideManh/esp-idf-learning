# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/ka/esp/esp-idf/components/bootloader/subproject"
  "/Users/ka/esp/i2c_self_test/build/bootloader"
  "/Users/ka/esp/i2c_self_test/build/bootloader-prefix"
  "/Users/ka/esp/i2c_self_test/build/bootloader-prefix/tmp"
  "/Users/ka/esp/i2c_self_test/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/ka/esp/i2c_self_test/build/bootloader-prefix/src"
  "/Users/ka/esp/i2c_self_test/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ka/esp/i2c_self_test/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
