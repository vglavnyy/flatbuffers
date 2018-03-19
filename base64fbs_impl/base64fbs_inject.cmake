# Written in 2017-2018 by vglavnyy.
#
# To the extent possible under law, the author(s) have dedicated all
# copyright and related and neighboring rights to this software to the
# public domain worldwide. This software is distributed without any warranty.
#
# You should have received a copy of the CC0 Public Domain Dedication
# along with this software. If not, see
# http://creativecommons.org/publicdomain/zero/1.0/
#

add_library(base64fbs_impl OBJECT "${CMAKE_CURRENT_LIST_DIR}/src/base64_impl.cpp")
target_include_directories(base64fbs_impl PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
set_property(TARGET ${base64fbs_impl} PROPERTY POSITION_INDEPENDENT_CODE ON)

# get transitive properties (if external base64 library will be used)
# https://stackoverflow.com/questions/38832528/transitive-target-include-directories-on-object-libraries
get_property(base64fbs_include_dirs TARGET base64fbs_impl PROPERTY INCLUDE_DIRECTORIES)
get_property(base64fbs_link_libs TARGET base64fbs_impl PROPERTY LINK_LIBRARIES)

macro(add_base64fbs impl_lib target)
  if(TARGET ${target})
    target_sources(${target} PRIVATE $<TARGET_OBJECTS:${impl_lib}>)
    target_include_directories(${target} PRIVATE ${base64fbs_include_dirs})
    target_link_libraries(${target} PRIVATE ${base64fbs_link_libs})
  endif()
endmacro(add_base64fbs)

add_base64fbs(base64fbs_impl flatbuffers)
add_base64fbs(base64fbs_impl flatsamplebinary)
add_base64fbs(base64fbs_impl flatsampletext)

if(MSVC AND TARGET flatc)
  add_library(base64fbs_impl_flatc OBJECT "${CMAKE_CURRENT_LIST_DIR}/src/base64_impl.cpp")
  #target_include_directories(base64fbs_impl_flatc PUBLIC "${CMAKE_CURRENT_LIST_DIR}/include")
  target_compile_options(base64fbs_impl_flatc PUBLIC $<$<CONFIG:Release>:/MT>)
  add_base64fbs(base64fbs_impl_flatc flatc)
else()
  add_base64fbs(base64fbs_impl flatc)
endif()

if(TARGET flattests)
  add_base64fbs(base64fbs_impl flattests)
  add_library(base64fbs_test OBJECT "${CMAKE_CURRENT_LIST_DIR}/src/base64_test.cpp")
  target_include_directories(base64fbs_test PRIVATE "${CMAKE_CURRENT_LIST_DIR}/include")
  target_sources(flattests PRIVATE $<TARGET_OBJECTS:base64fbs_test>)
endif()
