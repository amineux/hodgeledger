cmake_minimum_required(VERSION 3.20)
project(hodgeledger VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)

set(HODGELEDGER_SOURCES
    src/sparse.cpp
    src/csv.cpp
    src/json.cpp
    src/complex.cpp
    src/hodge.cpp
    src/lanczos.cpp
    src/embedding.cpp
    src/harmonic.cpp
    src/bridges.cpp
    src/timeline.cpp
    src/ledger.cpp
    src/io.cpp
)

add_library(hodgeledger_core STATIC ${HODGELEDGER_SOURCES})
target_include_directories(hodgeledger_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_features(hodgeledger_core PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(hodgeledger_core PRIVATE /W4)
else()
    target_compile_options(hodgeledger_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(hodgeledger src/main.cpp)
target_link_libraries(hodgeledger PRIVATE hodgeledger_core)

include(CTest)
option(HODGELEDGER_FETCH_GTEST "Fetch GoogleTest" ON)

if(BUILD_TESTING)
    set(_have_gtest FALSE)
    if(HODGELEDGER_FETCH_GTEST)
        include(FetchContent)
        FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
        set(_have_gtest TRUE)
    endif()

    if(_have_gtest)
        add_executable(hodgeledger_tests
            tests/test_sparse.cpp
            tests/test_complex.cpp
            tests/test_hodge.cpp
            tests/test_lanczos.cpp
            tests/test_harmonic.cpp
            tests/test_bridges.cpp
            tests/test_ledger.cpp
            tests/test_csv.cpp
            tests/test_timeline.cpp
        )
        target_link_libraries(hodgeledger_tests PRIVATE hodgeledger_core GTest::gtest_main)
        target_compile_definitions(hodgeledger_tests PRIVATE
            HODGELEDGER_FIXTURE_DIR="${CMAKE_SOURCE_DIR}/data/fixtures")
        include(GoogleTest)
        gtest_discover_tests(hodgeledger_tests
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            DISCOVERY_TIMEOUT 60)
    endif()
endif()

find_program(COBC_EXECUTABLE cobc)
if(COBC_EXECUTABLE)
    set(COBOL_BIN_DIR ${CMAKE_BINARY_DIR}/cobol)
    file(MAKE_DIRECTORY ${COBOL_BIN_DIR})
    add_custom_target(cobol-ledger
        COMMAND ${COBC_EXECUTABLE} -free -I ${CMAKE_SOURCE_DIR}/cobol/copy -x
                -o ${COBOL_BIN_DIR}/post-flow
                ${CMAKE_SOURCE_DIR}/cobol/src/POST-FLOW.cbl
        COMMAND ${COBC_EXECUTABLE} -free -I ${CMAKE_SOURCE_DIR}/cobol/copy -x
                -o ${COBOL_BIN_DIR}/trial-balance
                ${CMAKE_SOURCE_DIR}/cobol/src/TRIAL-BALANCE.cbl
        COMMAND ${COBC_EXECUTABLE} -free -I ${CMAKE_SOURCE_DIR}/cobol/copy -x
                -o ${COBOL_BIN_DIR}/report
                ${CMAKE_SOURCE_DIR}/cobol/src/REPORT.cbl
        COMMENT "Compiling GnuCOBOL multi-dimensional ledger"
        VERBATIM)
    message(STATUS "cobc found: ${COBC_EXECUTABLE} (target cobol-ledger available)")
else()
    message(STATUS "cobc not found — COBOL sources ship with scripts/verify_ledger.py")
endif()

install(TARGETS hodgeledger RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
