cmake_minimum_required(VERSION 3.0.0)
project(NES_Emulator VERSION 0.1.0)

add_compile_options(-std=c++17 -g)

include(CTest)
enable_testing()

find_package(SDL2 REQUIRED)



# See "https://github.com/google/googletest/tree/master/googletest"
# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)

# Now simply link against gtest or gtest_main as needed. Eg
# add_executable(example example.cpp)
# target_link_libraries(example gtest_main)
# add_test(NAME example_test COMMAND example)



set(gtest false) # set to false to disable unit testing
set(debug false)  # set to false to omit logging part of code

if(debug)
    add_compile_definitions(DEBUG)
endif(debug)

add_library(Cartridge SHARED include/Cartridge.hpp src/Cartridge.cpp)
add_library(Mapper SHARED include/Mapper.hpp src/Mapper.cpp)
add_library(Memory SHARED include/Memory.hpp src/Memory.cpp)
add_library(RICOH2A03 SHARED include/Ricoh2A03.hpp src/Ricoh2A03.cpp)
add_library(RICOH2C02 SHARED include/Ricoh2C02.hpp src/Ricoh2C02.cpp)
add_library(IO SHARED include/IO.hpp src/IO.cpp)

if(gtest)
    add_library(GtestModules SHARED testModules/gtestModules.hpp)
    set_target_properties(GtestModules PROPERTIES LINKER_LANGUAGE CXX)
    add_compile_definitions(GTEST)
endif(gtest)

target_link_libraries(Mapper PRIVATE Cartridge)
target_link_libraries(Memory PRIVATE Mapper)
target_link_libraries(RICOH2A03 PRIVATE Memory)
target_link_libraries(RICOH2C02 PRIVATE Memory)
target_link_libraries(IO PRIVATE Memory)
target_link_libraries(IO PRIVATE SDL2::SDL2)

add_executable(NES_Emulator main.cpp)

if(gtest)
    target_link_libraries(NES_Emulator PRIVATE Mapper Memory RICOH2A03 RICOH2C02 IO gtest)
else()
    target_link_libraries(NES_Emulator PRIVATE Mapper Memory RICOH2A03 RICOH2C02 IO SDL2::SDL2)
endif(gtest)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
