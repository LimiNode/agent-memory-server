cmake_minimum_required(VERSION 3.20)

project(agent-memory-server-vs01 LANGUAGES CXX)

option(VS01_BUILD_CORE_SMOKE
    "Build the opt-in VS-01 core API compile/link/reopen smoke test"
    OFF)
set(AGENT_MEMORY_CPP_SOURCE_DIR "" CACHE PATH
    "Path to the audited agent-memory-cpp checkout")

if(NOT VS01_BUILD_CORE_SMOKE)
    message(STATUS
        "VS-01 smoke test disabled; set VS01_BUILD_CORE_SMOKE=ON and "
        "AGENT_MEMORY_CPP_SOURCE_DIR to an audited core checkout")
    return()
endif()

if("${AGENT_MEMORY_CPP_SOURCE_DIR}" STREQUAL "" OR
   NOT EXISTS "${AGENT_MEMORY_CPP_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "VS01_BUILD_CORE_SMOKE requires AGENT_MEMORY_CPP_SOURCE_DIR pointing "
        "to an agent-memory-cpp checkout")
endif()

set(AGENT_MEMORY_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(AGENT_MEMORY_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(AGENT_MEMORY_BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(AGENT_MEMORY_ENABLE_JSON OFF CACHE BOOL "" FORCE)
set(AGENT_MEMORY_ENABLE_MDBX ON CACHE BOOL "" FORCE)
set(AGENT_MEMORY_MDBX_DEPS_MODE BUNDLED CACHE STRING "" FORCE)

add_subdirectory(
    "${AGENT_MEMORY_CPP_SOURCE_DIR}"
    "${CMAKE_BINARY_DIR}/agent-memory-cpp"
    EXCLUDE_FROM_ALL
)

add_executable(agent_memory_server_vs01_core_smoke
    tests/vs01_core_smoke.cpp
)
target_compile_features(agent_memory_server_vs01_core_smoke PRIVATE cxx_std_17)
target_link_libraries(agent_memory_server_vs01_core_smoke
    PRIVATE agent_memory::agent_memory)

enable_testing()
add_test(NAME agent_memory_server_vs01_core_smoke
    COMMAND agent_memory_server_vs01_core_smoke)
