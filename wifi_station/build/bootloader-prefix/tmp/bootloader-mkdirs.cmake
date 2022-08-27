# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/Users/ka/esp/esp-idf/components/bootloader/subproject"
  "/Users/ka/esp/wifi_station/build/bootloader"
  "/Users/ka/esp/wifi_station/build/bootloader-prefix"
  "/Users/ka/esp/wifi_station/build/bootloader-prefix/tmp"
  "/Users/ka/esp/wifi_station/build/bootloader-prefix/src/bootloader-stamp"
  "/Users/ka/esp/wifi_station/build/bootloader-prefix/src"
  "/Users/ka/esp/wifi_station/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/ka/esp/wifi_station/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
