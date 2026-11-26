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
target_link_libraries(oee_geb_core PUBLIC oee_bff_core Threads::Threads)

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
add_executable(netdump tools/netdump/main.cpp)
target_link_libraries(netdump PRIVATE oee_geb_core)
add_executable(signals tools/signals/main.cpp)
target_link_libraries(signals PRIVATE oee_geb_core)
add_executable(tourney tools/tourney/main.cpp)
target_link_libraries(tourney PRIVATE oee_geb_core)
add_test(NAME mspd_sanity COMMAND python3 -I ${CMAKE_SOURCE_DIR}/tests/test_mspd.py)

# M3: BFF primordial soup (arXiv:2406.19108), bit-exact with cubff bff_noheads.
find_library(BROTLIENC brotlienc REQUIRED)
find_package(Threads REQUIRED)
add_library(oee_bff_core src/bff/soup.cpp)
target_include_directories(oee_bff_core PUBLIC src)
target_link_libraries(oee_bff_core PUBLIC ${BROTLIENC} Threads::Threads)
add_executable(oee_bff apps/oee_bff/main.cpp)
target_link_libraries(oee_bff PRIVATE oee_bff_core)
add_executable(test_egc tests/bff/test_egc.cpp)
target_link_libraries(test_egc PRIVATE oee_bff_core)
add_test(NAME bff_egc COMMAND test_egc)
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

# L0: mass-conserving continuous CA with parameters carried by matter (MaCE transport + Flow-Lenia localisation).
add_library(oee_l0_core src/l0/world.cpp)
target_include_directories(oee_l0_core PUBLIC src)
add_executable(test_l0 tests/l0/test_l0.cpp)
target_link_libraries(test_l0 PRIVATE oee_l0_core)
add_test(NAME l0_core COMMAND test_l0)
add_executable(oee_l0 apps/oee_l0/main.cpp)
target_link_libraries(oee_l0 PRIVATE oee_l0_core)

# Evolved models (docs/EVO.md): AMX int8 kernels
add_library(oee_evo_core src/evo/gemm.cpp src/evo/egg.cpp)
target_include_directories(oee_evo_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
set_source_files_properties(src/evo/gemm.cpp PROPERTIES COMPILE_OPTIONS "-mamx-tile;-mamx-int8")
# The EGG element-wise integer loops need vectorisation; built for the host CPU (turn off for portable binaries).
option(OEE_NATIVE "Build the evolved-model core for the host CPU (-O3 -march=native)" ON)
if(OEE_NATIVE)
  set_source_files_properties(src/evo/egg.cpp PROPERTIES COMPILE_OPTIONS "-O3;-march=native")
endif()
find_package(Threads REQUIRED)
target_link_libraries(oee_evo_core PUBLIC Threads::Threads)
add_executable(test_evo_gemm tests/evo/test_gemm.cpp)
target_link_libraries(test_evo_gemm PRIVATE oee_evo_core)
add_test(NAME evo_gemm COMMAND test_evo_gemm)
add_executable(test_evo_egg tests/evo/test_egg.cpp)
target_link_libraries(test_evo_egg PRIVATE oee_evo_core)
add_test(NAME evo_egg COMMAND test_evo_egg)
add_executable(oee_egg apps/oee_egg/main.cpp)
target_link_libraries(oee_egg PRIVATE oee_evo_core)
# World v1 (docs/WORLD.md)
add_library(oee_w_core src/w/world.cpp)
target_include_directories(oee_w_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
add_executable(oee_w apps/oee_w/main.cpp)
target_link_libraries(oee_w PRIVATE oee_w_core oee_evo_core)
add_executable(mcal tools/w/mcal.cpp)
target_link_libraries(mcal PRIVATE oee_w_core)
add_executable(test_w_world tests/w/test_world.cpp)
target_link_libraries(test_w_world PRIVATE oee_w_core)
add_test(NAME w_world COMMAND test_w_world)
# resume is bit-exact and independent of the thread count. W1 settings with splits, ancestors, extinction of the
# lineage in slot 0 with no split refilling it (crashed before 2026-11-25) and a reseed after all lineages died.
add_test(NAME w_resume
  COMMAND sh -c "B=$<TARGET_FILE:oee_w>; C='-pairs 4 -H 20 -L 12 -S 16 -founders 6 -lineages 3 -nact 7 -hist_frac 2 -hist_founders 4 -archive_every 2 -seed 3 -regrow 1 -base 2 -ck_every 4'; rm -f wr_a.ck wr_b.ck; $B $C -epochs 30 -threads 1 -ck wr_a.ck -log wr_a.txt && $B $C -epochs 12 -threads 2 -ck wr_b.ck -log wr_b.txt && $B $C -epochs 30 -threads 3 -ck wr_b.ck -log wr_b.txt && cmp wr_a.ck wr_b.ck && grep -v '^#' wr_a.txt | cut -f1-7,9- > wr_a.cut && grep -v '^#' wr_b.txt | cut -f1-7,9- > wr_b.cut && cmp wr_a.cut wr_b.cut && test $(grep -c -v '^#' wr_a.txt) = 30 && awk '!/^#/{r=$9;x=$13}END{exit !(r>0&&x>0)}' wr_a.txt")
# W1e (an ancestor per fork pair) and W1b (split lineages keep the parent's age): resume stays bit-exact
add_test(NAME w1e_resume
  COMMAND sh -c "B=$<TARGET_FILE:oee_w>; C='-pairs 4 -H 20 -L 12 -S 16 -founders 6 -lineages 3 -nact 7 -hist_frac 2 -hist_pair 1 -split_age 1 -hist_founders 4 -archive_every 2 -seed 3 -regrow 1 -base 2 -ck_every 4'; rm -f we_a.ck we_b.ck; $B $C -epochs 24 -threads 1 -ck we_a.ck -log we_a.txt && $B $C -epochs 12 -threads 2 -ck we_b.ck -log we_b.txt && $B $C -epochs 24 -threads 3 -ck we_b.ck -log we_b.txt && cmp we_a.ck we_b.ck && grep -v '^#' we_a.txt | cut -f1-7,9- > we_a.cut && grep -v '^#' we_b.txt | cut -f1-7,9- > we_b.cut && cmp we_a.cut we_b.cut")
# W3 (programmable matter): resume stays bit-exact with machines in the world; the wipe ablation runs
add_test(NAME w3_resume
  COMMAND sh -c "B=$<TARGET_FILE:oee_w>; C='-pairs 4 -H 20 -L 12 -S 16 -founders 6 -lineages 2 -nact 10 -matter 1 -hist_frac 2 -split_age 1 -hist_founders 4 -archive_every 2 -seed 4 -regrow 4 -ck_every 4'; rm -rf w3r_a.ck w3r_b.ck w3a && mkdir -p w3a && $B $C -epochs 16 -threads 1 -ck w3r_a.ck -archive w3a -log w3r_a.txt && $B $C -epochs 8 -threads 2 -ck w3r_b.ck -log w3r_b.txt && $B $C -epochs 16 -threads 3 -ck w3r_b.ck -log w3r_b.txt && cmp w3r_a.ck w3r_b.ck && grep -v '^#' w3r_a.txt | cut -f1-7,9- > w3r_a.cut && grep -v '^#' w3r_b.txt | cut -f1-7,9- > w3r_b.cut && cmp w3r_a.cut w3r_b.cut && $B -tourney w3a/egg_16.bin w3a/egg_16.bin -seeds 2 -ticks 300 -L 12 -S 16 -founders 6 -nact 10 -matter 1 -regrow 4 -ablate wipe | grep -q '^mean_ishareA'")
# W3 generality arenas: generated settings and random programs; a genome against itself scores exactly 0.5, alone
# exactly log ratio 0
add_test(NAME w3_generality
  COMMAND sh -c "B=$<TARGET_FILE:oee_w>; rm -rf wg && mkdir -p wg && $B -epochs 2 -pairs 4 -H 20 -L 16 -S 16 -founders 6 -lineages 2 -nact 10 -matter 1 -archive wg -archive_every 1 -log wg/log.txt && G='-seeds 3 -ticks 600 -S 16 -founders 6 -nact 10 -matter 1 -gen 1 -arena_progs 24' && $B -tourney wg/egg_2.bin wg/egg_2.bin $G > wg/t.txt && grep -q '^mean_lshareA 0.5000' wg/t.txt && $B -tourney wg/egg_2.bin wg/egg_2.bin $G -solo 1 > wg/s.txt && grep -q '^solo_log_ratio 0.0000' wg/s.txt && $B -tourney wg/egg_2.bin wg/egg_2.bin $G -solo 1 -swap_at 300 -swap_win 200 > wg/w.txt && grep -q '^swap_recovery 0.0000' wg/w.txt")
# wave 2 (kin-only signals, Lamarckian state, extinction events, stress-induced update fraction): resume stays bit-exact
add_test(NAME wave2_resume
  COMMAND sh -c "B=$<TARGET_FILE:oee_w>; C='-pairs 4 -H 20 -L 12 -S 16 -founders 6 -lineages 3 -nact 10 -matter 1 -spawn 20 -mut 1024 -own 1 -nodes 8 -signals 1 -kin_sig 1 -lamarck 1 -wall_decay 64 -m_cost 4 -m_give 16 -graze 1 -pass 1 -cat_every 5 -cat_keep 0.2 -stress 4 -hist_frac 2 -split_age 1 -hist_founders 4 -archive_every 2 -seed 5 -regrow 8 -ck_every 4'; rm -f wv_a.ck wv_b.ck; $B $C -epochs 20 -threads 1 -ck wv_a.ck -log wv_a.txt && $B $C -epochs 8 -threads 2 -ck wv_b.ck -log wv_b.txt && $B $C -epochs 20 -threads 3 -ck wv_b.ck -log wv_b.txt && cmp wv_a.ck wv_b.ck && grep -v '^#' wv_a.txt | cut -f1-7,9- > wv_a.cut && grep -v '^#' wv_b.txt | cut -f1-7,9- > wv_b.cut && cmp wv_a.cut wv_b.cut")
# mirrored tournament: a genome against itself scores exactly 0.5 (all ticks and after founders); W2a ablations run
add_test(NAME w_tourney_mirror
  COMMAND sh -c "B=$<TARGET_FILE:oee_w>; rm -rf wt_a && mkdir -p wt_a && $B -epochs 2 -pairs 4 -H 20 -L 16 -S 16 -founders 6 -lineages 2 -nact 7 -season 40 -view 1 -signals 1 -regrow 16 -archive wt_a -archive_every 1 -log wt_a/log.txt && $B -tourney wt_a/egg_2.bin wt_a/egg_2.bin -seeds 2 -ticks 700 -L 16 -S 16 -founders 6 -nact 7 -season 40 -view 1 -signals 1 -regrow 16 -climate_ticks 300 > wt_a/t.txt && grep -q '^mean_ishareA 0.5000' wt_a/t.txt && grep -q '^mean_lshareA 0.5000' wt_a/t.txt && $B -tourney wt_a/egg_2.bin wt_a/egg_2.bin -seeds 2 -ticks 300 -L 16 -S 16 -founders 6 -nact 7 -season 40 -view 1 -signals 1 -regrow 16 -ablate deaf | grep -q '^mean_ishareA'")
# same for W2a (seasons with climate redraws across the checkpoint, hidden resources, signals)
add_test(NAME w2_resume
  COMMAND sh -c "B=$<TARGET_FILE:oee_w>; C='-pairs 4 -H 20 -L 12 -S 16 -founders 6 -lineages 3 -nact 7 -hist_frac 2 -hist_founders 4 -archive_every 2 -seed 3 -regrow 6 -season 30 -view 1 -signals 1 -climate_every 3 -ck_every 4'; rm -f w2r_a.ck w2r_b.ck; $B $C -epochs 30 -threads 1 -ck w2r_a.ck -log w2r_a.txt && $B $C -epochs 12 -threads 2 -ck w2r_b.ck -log w2r_b.txt && $B $C -epochs 30 -threads 3 -ck w2r_b.ck -log w2r_b.txt && cmp w2r_a.ck w2r_b.ck && grep -v '^#' w2r_a.txt | cut -f1-7,9- > w2r_a.cut && grep -v '^#' w2r_b.txt | cut -f1-7,9- > w2r_b.cut && cmp w2r_a.cut w2r_b.cut && test $(grep -c -v '^#' w2r_a.txt) = 30")
