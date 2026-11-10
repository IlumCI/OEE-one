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
target_link_libraries(oee_geb_core PUBLIC oee_bff_core)

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
add_test(NAME mspd_sanity COMMAND python3 -I ${CMAKE_SOURCE_DIR}/tests/test_mspd.py)

# M3: BFF primordial soup (arXiv:2406.19108), bit-exact with cubff bff_noheads.
find_library(BROTLIENC brotlienc REQUIRED)
find_package(Threads REQUIRED)
add_library(oee_bff_core src/bff/soup.cpp)
target_include_directories(oee_bff_core PUBLIC src)
target_link_libraries(oee_bff_core PUBLIC ${BROTLIENC} Threads::Threads)
add_executable(oee_bff apps/oee_bff/main.cpp)
target_link_libraries(oee_bff PRIVATE oee_bff_core)
add_test(NAME bff_cubff_equivalence
  COMMAND sh -c "$<TARGET_FILE:oee_bff> -seed 10248 -epochs 256 -interval 64 -threads 2 -cubff-log -log bff_eq.txt && diff -u ${CMAKE_SOURCE_DIR}/tests/golden/bff/cubff_bff_noheads_seed10248.txt bff_eq.txt")
add_test(NAME bff_resume
  COMMAND sh -c "B=$<TARGET_FILE:oee_bff>; $B -seed 7 -programs 4096 -epochs 200 -interval 50 -save r_a.ck -log /dev/null && $B -seed 7 -programs 4096 -epochs 100 -interval 50 -save r_b.ck -log /dev/null && $B -seed 7 -programs 4096 -epochs 200 -interval 50 -resume r_b.ck -save r_b.ck -log /dev/null && cmp r_a.ck r_b.ck")
add_executable(bff_inspect apps/oee_bff/inspect.cpp)
target_link_libraries(bff_inspect PRIVATE oee_bff_core)
add_executable(test_bff_selfrep tests/bff/test_selfrep.cpp)
target_link_libraries(test_bff_selfrep PRIVATE oee_bff_core)
add_test(NAME bff_selfrep_controls COMMAND test_bff_selfrep)
add_test(NAME bff_spatial_resume
  COMMAND sh -c "B=$<TARGET_FILE:oee_bff>; $B -seed 3 -grid 64 32 -epochs 200 -interval 50 -save g_a.ck -log /dev/null && $B -seed 3 -grid 64 32 -epochs 100 -interval 50 -save g_b.ck -log /dev/null && $B -epochs 200 -interval 50 -resume g_b.ck -save g_b.ck -log /dev/null && cmp g_a.ck g_b.ck")
add_executable(test_bff_evaluate_n tests/bff/test_evaluate_n.cpp)
target_link_libraries(test_bff_evaluate_n PRIVATE oee_bff_core)
add_test(NAME bff_evaluate_n_equivalence COMMAND test_bff_evaluate_n)
add_executable(m5_heritability tools/m5/heritability.cpp)
target_link_libraries(m5_heritability PRIVATE oee_geb_core)
