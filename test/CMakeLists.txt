# Tests für OpenSylab v0.2

# Sammle alle Test-Quellen
set(TEST_SOURCES
    unit/test_sample.cpp
    unit/test_order.cpp
    unit/test_testresult.cpp
    unit/test_database.cpp
    unit/test_api.cpp
    unit/test_csvimport.cpp
    unit/test_csvresultimport.cpp
    unit/test_hl7.cpp
    unit/test_fhir.cpp
    unit/test_statistics.cpp
    unit/test_utils.cpp
    unit/test_runner.cpp
)

# Test-Executable erstellen
add_executable(opensylab_tests ${TEST_SOURCES})

# Include-Verzeichnisse
target_include_directories(opensylab_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Libraries verlinken
target_link_libraries(opensylab_tests PRIVATE SQLite::SQLite3 OpenSSL::SSL OpenSSL::Crypto jwt-cpp::jwt-cpp)

# Quellcode-Dateien die getestet werden sollen
target_sources(opensylab_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src/core/Sample.cpp
    ${PROJECT_SOURCE_DIR}/src/core/Order.cpp
    ${PROJECT_SOURCE_DIR}/src/core/TestResult.cpp
    ${PROJECT_SOURCE_DIR}/src/core/AuditEntry.cpp
    ${PROJECT_SOURCE_DIR}/src/core/User.cpp
    ${PROJECT_SOURCE_DIR}/src/db/Database.cpp
    ${PROJECT_SOURCE_DIR}/src/api/ApiServer.cpp
    ${PROJECT_SOURCE_DIR}/src/api/TlsContext.cpp
    ${PROJECT_SOURCE_DIR}/src/auth/JwtAuth.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/CsvImport.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/CsvResultImport.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/Hl7.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/Fhir.cpp
)

# Test hinzufügen
add_test(NAME opensylab_unit_tests COMMAND opensylab_tests)

message(STATUS "Test framework konfiguriert - Ausführen mit 'make test' oder 'ctest'")
