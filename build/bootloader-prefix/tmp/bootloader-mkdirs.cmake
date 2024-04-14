# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "E:/esp/v5.2.1/esp-idf/components/bootloader/subproject"
  "E:/esp/main/build/bootloader"
  "E:/esp/main/build/bootloader-prefix"
  "E:/esp/main/build/bootloader-prefix/tmp"
  "E:/esp/main/build/bootloader-prefix/src/bootloader-stamp"
  "E:/esp/main/build/bootloader-prefix/src"
  "E:/esp/main/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/esp/main/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/esp/main/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
