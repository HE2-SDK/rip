cmake_minimum_required(VERSION 3.28)
project(rip VERSION 1.0.37
                 DESCRIPTION "Restoration Issue Pocketknife"
                 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(yyjson GIT_REPOSITORY https://github.com/ibireme/yyjson.git GIT_TAG c3af82ba8d72125469d956fe8c9f0de5b3b8f5e7 EXCLUDE_FROM_ALL)
FetchContent_Declare(reflectcpp GIT_REPOSITORY https://github.com/getml/reflect-cpp.git GIT_TAG v0.16.0 EXCLUDE_FROM_ALL)
FetchContent_Declare(CLI11 GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git GIT_TAG v2.4.2 EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 2.4)
FetchContent_Declare(universal-cslib GIT_REPOSITORY https://github.com/angryzor/universal-cslib.git GIT_TAG v1.0.52 EXCLUDE_FROM_ALL FIND_PACKAGE_ARGS 1.0.52)
FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git GIT_TAG v3.8.1)
FetchContent_MakeAvailable(yyjson reflectcpp CLI11 universal-cslib Catch2)

add_subdirectory(rip)
add_subdirectory(rip-hl)

# This works around a bug in reflectcpp's CMakeLists.txt. It incorrectly adds an include dir in the build tree, which does not exist.
file(MAKE_DIRECTORY ${reflectcpp_BINARY_DIR}/include)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(rip-config-version.cmake VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
configure_file(cmake/rip-config.cmake "${CMAKE_CURRENT_BINARY_DIR}/rip-config.cmake" COPYONLY)
export(TARGETS rip rip-hl yyjson reflectcpp FILE rip-targets.cmake)
export(PACKAGE rip)
