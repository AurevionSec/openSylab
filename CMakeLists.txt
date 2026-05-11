cmake_minimum_required(VERSION 3.15)

# Projektdefinition
project(OpenSylab
    VERSION 0.7.0
    DESCRIPTION "Open Source Laboratory Information Management System (LIMS)"
    LANGUAGES CXX
)

# Version-Header aus Template generieren (SSOT: VERSION in project() oben)
configure_file(
    "${CMAKE_SOURCE_DIR}/include/version.h.in"
    "${CMAKE_SOURCE_DIR}/include/version.h"
    @ONLY
)

# C++ Standard festlegen
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build-Typ festlegen (falls nicht gesetzt)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# Ausgabeverzeichnisse
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Include-Verzeichnisse
include_directories(${PROJECT_SOURCE_DIR}/include)

# Compiler-Optionen
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -pedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
elseif(MSVC)
    add_compile_options(/W4)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(/Zi /Od)
    else()
        add_compile_options(/O2)
    endif()
endif()

# SQLite3 finden oder einbinden
find_package(SQLite3 REQUIRED)
if(SQLite3_FOUND)
    message(STATUS "Found SQLite3: ${SQLite3_INCLUDE_DIRS}")
    include_directories(${SQLite3_INCLUDE_DIRS})
else()
    message(FATAL_ERROR "SQLite3 not found. Please install libsqlite3-dev")
endif()

# OpenSSL finden (für TLS/HTTPS Support)
find_package(OpenSSL REQUIRED)
if(OpenSSL_FOUND)
    message(STATUS "Found OpenSSL: ${OPENSSL_VERSION}")
    message(STATUS "OpenSSL Include: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL Libraries: ${OPENSSL_LIBRARIES}")
    include_directories(${OPENSSL_INCLUDE_DIR})
else()
    message(FATAL_ERROR "OpenSSL not found. Please install libssl-dev")
endif()

# JWT-CPP (für JWT Token Authentication)
include(FetchContent)
FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG        v0.7.0
)
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "")
set(JWT_BUILD_TESTS OFF CACHE BOOL "")
FetchContent_MakeAvailable(jwt-cpp)
message(STATUS "JWT-CPP: Fetched successfully")

# Quellcode-Dateien sammeln
set(CORE_SOURCES
    src/core/Sample.cpp
    src/core/Order.cpp
    src/core/TestResult.cpp
    src/core/AuditEntry.cpp
    src/core/User.cpp
)

set(DB_SOURCES
    src/db/Database.cpp
)

set(UTILS_SOURCES
    src/utils/CsvImport.cpp
    src/utils/CsvResultImport.cpp
    src/utils/Hl7.cpp
    src/utils/Fhir.cpp
    src/utils/CliInterface.cpp
)

set(API_SOURCES
    src/api/ApiServer.cpp
    src/api/TlsContext.cpp
)

set(AUTH_SOURCES
    src/auth/JwtAuth.cpp
)

set(ALL_SOURCES
    ${CORE_SOURCES}
    ${DB_SOURCES}
    ${UTILS_SOURCES}
    ${API_SOURCES}
    ${AUTH_SOURCES}
    src/main.cpp
)

# Executable erstellen
add_executable(${PROJECT_NAME} ${ALL_SOURCES})

# Libraries verlinken
target_link_libraries(${PROJECT_NAME} PRIVATE
    SQLite::SQLite3
    OpenSSL::SSL
    OpenSSL::Crypto
    jwt-cpp::jwt-cpp
)

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
install(DIRECTORY config/ DESTINATION share/${PROJECT_NAME}/config)
install(DIRECTORY docs/ DESTINATION share/${PROJECT_NAME}/docs)

# Testing aktivieren
enable_testing()
add_subdirectory(test)

# Informationen ausgeben
message(STATUS "")
message(STATUS "OpenSylab ${PROJECT_VERSION} - Configuration Summary")
message(STATUS "================================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "SQLite3: ${SQLite3_LIBRARIES}")
message(STATUS "OpenSSL: ${OPENSSL_VERSION}")
message(STATUS "")
