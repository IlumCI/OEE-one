cmake_minimum_required(VERSION 3.25)
project(oee_one LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Bit-exact reproduction of the reference requires IEEE semantics: no fast-math,
# no excess precision, no contraction of a*b+c into FMA.
add_compile_options(-Wall -Wextra -ffp-contract=off -fno-fast-math)

option(OEE_SANITIZE "Build with AddressSanitizer and UBSan" OFF)
if(OEE_SANITIZE)
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
  add_link_options(-fsanitize=address,undefined)
endif()

add_library(oee_geb_core
  src/geb/rng.cpp
  src/geb/ann.cpp
  src/geb/world.cpp
  src/geb/rulestat.cpp)
target_include_directories(oee_geb_core PUBLIC src)

add_executable(oee_geb_resets apps/oee_geb/main.cpp)
target_link_libraries(oee_geb_resets PRIVATE oee_geb_core)
target_compile_definitions(oee_geb_resets PRIVATE OEE_GEB_DEFAULT_RESETS=1)

add_executable(oee_geb_no_resets apps/oee_geb/main.cpp)
target_link_libraries(oee_geb_no_resets PRIVATE oee_geb_core)
target_compile_definitions(oee_geb_no_resets PRIVATE OEE_GEB_DEFAULT_RESETS=0)

enable_testing()
add_test(NAME golden_equivalence
  COMMAND ${CMAKE_SOURCE_DIR}/tests/check_golden.sh
          ${CMAKE_BINARY_DIR}/oee_geb ${CMAKE_SOURCE_DIR}/tests/golden/geb_v08b3
          ${CMAKE_BINARY_DIR}/golden_runs)
add_test(NAME activity_analyser COMMAND python3 -I ${CMAKE_SOURCE_DIR}/tests/test_activity.py)
add_test(NAME checkpoint_resume
  COMMAND ${CMAKE_SOURCE_DIR}/tests/check_resume.sh ${CMAKE_BINARY_DIR}/oee_geb ${CMAKE_BINARY_DIR}/resume_runs)
add_executable(bench_generate_rules tests/bench/bench_generate_rules.cpp)
target_link_libraries(bench_generate_rules PRIVATE oee_geb_core)
add_executable(inspect_checkpoint tools/inspect_checkpoint/main.cpp)
target_link_libraries(inspect_checkpoint PRIVATE oee_geb_core)
add_test(NAME rescue_rollback_reseed
  COMMAND ${CMAKE_SOURCE_DIR}/tests/check_rescue.sh ${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/rescue_runs)
add_executable(replay tools/replay/main.cpp)
target_link_libraries(replay PRIVATE oee_geb_core)
