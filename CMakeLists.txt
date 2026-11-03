# =============================================================================
#  DSA Series - auto-discovering CMakeLists.txt        (place in the project root)
#
#  RULES
#    1. Any folder named "Projects" (at any depth): every sub-folder inside it is
#       ONE multi-file project  ->  one executable built from all .cpp files in it.
#           Array/Projects/ATM Simulator/{main.cpp, credential.h}  ->  ATM_Simulator
#    2. Every other .cpp/.cc/.cxx file is its own executable (each has its own main).
#           Array/Practise Quest/Min Max.cpp                        ->  Min_Max
#    3. New files / folders are picked up automatically - no editing needed.
#       (CLion: Tools > CMake > Reload CMake Project, if it doesn't refresh itself.)
#    4. A folder that has its own CMakeLists.txt is handed over to it (add_subdirectory).
#    5. Test input without retyping: put "<program name>.in" next to a single-file program
#       (Min Max.cpp -> Min Max.in) or "input.txt" in a project folder. If the file exists,
#       stdin is read from it automatically; if not, nothing changes (type in the console).
#
#  Change any option below with -D, e.g. in CLion:
#    Settings > Build, Execution, Deployment > CMake > CMake options:  -DDSA_USE_PCH=ON
# =============================================================================
cmake_minimum_required(VERSION 3.20)
project(DSA_Series LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-source builds are not supported. Use a separate build "
            "directory (CLion's cmake-build-* folders already do).")
endif()

# -----------------------------------------------------------------------------
#  1. SETTINGS
# -----------------------------------------------------------------------------
set(DSA_CXX_STANDARD 23 CACHE STRING "C++ standard to compile with (17, 20, 23)")
option(DSA_CXX_EXTENSIONS   "GNU extensions (-std=gnu++XX): keeps __int128, M_PI, etc. working on MinGW" ON)
option(DSA_WARNINGS         "Enable a sensible set of compiler warnings"                                  ON)
option(DSA_WERROR           "Treat warnings as errors"                                                    OFF)
option(DSA_DEBUG_CHECKS     "Debug builds: bounds-check vector/array/string access (libstdc++ assertions)" ON)
option(DSA_STATIC_RUNTIME   "MinGW only: link libstdc++/libgcc statically so the .exe runs outside CLion"  ON)
set(DSA_SANITIZE "" CACHE STRING "Sanitizers, e.g. 'undefined' or 'address,undefined' (empty = off; rarely available on MinGW)")
set(DSA_USE_PCH "AUTO" CACHE STRING "Shared precompiled <bits/stdc++.h>: AUTO = only programs that include it, ON = all, OFF = none")
set_property(CACHE DSA_USE_PCH PROPERTY STRINGS AUTO ON OFF)
option(DSA_AUTO_INPUT       "Read stdin from '<name>.in' / 'input.txt' next to the sources when that file exists" ON)
set(DSA_STACK_SIZE_MB 256 CACHE STRING "Windows only: stack size per program in MB (0 = toolchain default, only ~2 MB on MinGW)")
option(DSA_DEFINE_SOURCE_DIR "Define DSA_SOURCE_DIR (folder of the sources) for every program"           ON)
option(DSA_CLANG_TIDY       "Run clang-tidy on every compile (if installed)"                              OFF)
option(DSA_BUILD_ALL        "Include every program in 'Build All' (OFF = build only what you run)"        ON)
option(DSA_REQUIRE_MAIN     "Skip .cpp files that contain no main()"                                      ON)
option(DSA_VERBOSE          "Print the discovered program list while configuring"                         ON)
set(DSA_PROJECTS_DIR_NAME "Projects" CACHE STRING "Sub-folders of any folder with this name are multi-file projects")
set(DSA_EXCLUDE_REGEX "" CACHE STRING "Extra regex: relative paths matching it are ignored (e.g. 'Scratch|Old')")
set(DSA_SHARED_INCLUDE_DIRS "include;common" CACHE STRING "Root-level folders added to every program's include path (if they exist)")

# -----------------------------------------------------------------------------
#  2. LANGUAGE / BUILD-TYPE / OUTPUT
# -----------------------------------------------------------------------------
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11
        AND DSA_CXX_STANDARD GREATER 20)
    message(FATAL_ERROR "C++${DSA_CXX_STANDARD} needs GCC 11 or newer (found ${CMAKE_CXX_COMPILER_VERSION}). "
            "Update the MinGW toolchain in CLion or use -DDSA_CXX_STANDARD=20.")
endif()

set(CMAKE_CXX_STANDARD          ${DSA_CXX_STANDARD})
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        ${DSA_CXX_EXTENSIONS})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# -----------------------------------------------------------------------------
#  3. SHARED FLAGS  (every program links "dsa_options"; sub-projects can too)
# -----------------------------------------------------------------------------
add_library(dsa_options INTERFACE
        "Array/Practise Quest/f1.cpp")

if(DSA_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(dsa_options INTERFACE -Wall -Wextra -Wpedantic)
    elseif(MSVC)
        target_compile_options(dsa_options INTERFACE /W4)
    endif()
endif()

if(DSA_WERROR)
    if(MSVC)
        target_compile_options(dsa_options INTERFACE /WX)
    else()
        target_compile_options(dsa_options INTERFACE -Werror)
    endif()
endif()

if(DSA_DEBUG_CHECKS AND NOT MSVC)   # MSVC already has checked iterators in Debug
    target_compile_definitions(dsa_options INTERFACE "$<$<CONFIG:Debug>:_GLIBCXX_ASSERTIONS>")
endif()

if(MINGW AND DSA_STATIC_RUNTIME)
    target_link_options(dsa_options INTERFACE -static-libgcc -static-libstdc++)
endif()

# Deep recursion (DFS, backtracking, linked lists of 1e5 nodes ...) overflows MinGW's tiny default
# stack and the program just dies silently (exit code 0xC00000FD). Linux gives 8 MB by default.
if(WIN32 AND DSA_STACK_SIZE_MB GREATER 0)
    math(EXPR _stack_bytes "${DSA_STACK_SIZE_MB} * 1024 * 1024")
    if(MSVC)
        target_link_options(dsa_options INTERFACE "/STACK:${_stack_bytes}")
    else()
        target_link_options(dsa_options INTERFACE "-Wl,--stack,${_stack_bytes}")
    endif()
endif()

foreach(_inc IN LISTS DSA_SHARED_INCLUDE_DIRS)
    if(IS_DIRECTORY "${CMAKE_SOURCE_DIR}/${_inc}")
        target_include_directories(dsa_options INTERFACE "${CMAKE_SOURCE_DIR}/${_inc}")
    endif()
endforeach()

if(NOT "${DSA_SANITIZE}" STREQUAL "")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        include(CheckCXXSourceCompiles)
        string(MAKE_C_IDENTIFIER "DSA_SAN_OK_${DSA_SANITIZE}" _san_var)
        set(CMAKE_REQUIRED_FLAGS        "-fsanitize=${DSA_SANITIZE}")
        set(CMAKE_REQUIRED_LINK_OPTIONS "-fsanitize=${DSA_SANITIZE}")
        check_cxx_source_compiles("int main() { return 0; }" ${_san_var})
        unset(CMAKE_REQUIRED_FLAGS)
        unset(CMAKE_REQUIRED_LINK_OPTIONS)
        if(${_san_var})
            target_compile_options(dsa_options INTERFACE -fsanitize=${DSA_SANITIZE} -fno-omit-frame-pointer)
            target_link_options   (dsa_options INTERFACE -fsanitize=${DSA_SANITIZE})
        else()
            message(WARNING "DSA_SANITIZE='${DSA_SANITIZE}' is not supported by this toolchain - ignored.")
        endif()
    else()
        message(WARNING "DSA_SANITIZE is only wired up for GCC/Clang - ignored.")
    endif()
endif()

if(DSA_CLANG_TIDY)
    find_program(DSA_CLANG_TIDY_EXE NAMES clang-tidy)
    if(DSA_CLANG_TIDY_EXE)
        set(CMAKE_CXX_CLANG_TIDY "${DSA_CLANG_TIDY_EXE}")
    else()
        message(WARNING "DSA_CLANG_TIDY=ON but clang-tidy was not found - ignored.")
    endif()
endif()

# Precompiled <bits/stdc++.h>: built once (lazily, on first use) and shared by the programs.
# Saves ~2 s per compile, i.e. every time you press Run on a changed file.
set(_pch_possible FALSE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(_pch_possible TRUE)
elseif(NOT "${DSA_USE_PCH}" STREQUAL "AUTO" AND DSA_USE_PCH)
    message(WARNING "DSA_USE_PCH needs GCC/Clang - ignored.")
endif()

function(_dsa_ensure_pch)
    if(TARGET dsa_pch)
        return()
    endif()
    set(stub "${CMAKE_BINARY_DIR}/dsa_pch_stub.cpp")
    if(NOT EXISTS "${stub}")
        file(WRITE "${stub}" "// Generated: anchor for the shared precompiled header.\n")
    endif()
    add_library(dsa_pch STATIC EXCLUDE_FROM_ALL "${stub}")
    target_link_libraries(dsa_pch PRIVATE dsa_options)
    target_precompile_headers(dsa_pch PRIVATE <bits/stdc++.h>)
endfunction()

# -----------------------------------------------------------------------------
#  4. HELPERS
# -----------------------------------------------------------------------------
# Names CMake/Ninja/Make already use for their own targets (e.g. a file called test.cpp!).
set(_DSA_RESERVED all clean help install test package package_source edit_cache rebuild_cache
        list_install_components depend preinstall all_build run_tests zero_check
        dsa_options dsa_pch)

# "Min Max" -> Min_Max ; "136 Single Number" -> 136_Single_Number ; reserved names get _exe.
function(_dsa_target_name input out)
    string(REGEX REPLACE "[^A-Za-z0-9_]+" "_" s "${input}")
    string(REGEX REPLACE "^_+" "" s "${s}")
    string(REGEX REPLACE "_+$" "" s "${s}")
    if("${s}" STREQUAL "")
        set(s "program")
    endif()
    string(TOLOWER "${s}" s_lower)
    if(s_lower IN_LIST _DSA_RESERVED)
        set(s "${s}_exe")
    endif()
    set(${out} "${s}" PARENT_SCOPE)
endfunction()

# TRUE when "path" is "dir" or lies inside it (both relative to the project root).
function(_dsa_is_under path dir out)
    string(LENGTH "${dir}/" n)
    string(SUBSTRING "${path}/" 0 ${n} head)
    if("${head}" STREQUAL "${dir}/")
        set(${out} TRUE PARENT_SCOPE)
    else()
        set(${out} FALSE PARENT_SCOPE)
    endif()
endfunction()

# TRUE when the file contains something that looks like  main(
function(_dsa_has_main file out)
    file(STRINGS "${file}" hit REGEX "(^|[^A-Za-z0-9_])main[ \t]*\\(" LIMIT_COUNT 1)
    if("${hit}" STREQUAL "")
        set(${out} FALSE PARENT_SCOPE)
    else()
        set(${out} TRUE PARENT_SCOPE)
    endif()
endfunction()

# For "Array/Projects/ATM Simulator/sub" returns "Array/Projects/ATM Simulator".
# Returns "" if the directory is not inside <Projects>/<something>.
function(_dsa_find_project_root rel_dir out)
    set(${out} "" PARENT_SCOPE)
    if("${rel_dir}" STREQUAL "")
        return()
    endif()
    string(REPLACE "/" ";" segs "${rel_dir}")
    list(LENGTH segs count)
    string(TOLOWER "${DSA_PROJECTS_DIR_NAME}" key)
    set(i 0)
    foreach(seg IN LISTS segs)
        string(TOLOWER "${seg}" seg_lower)
        math(EXPR next "${i} + 1")
        if("${seg_lower}" STREQUAL "${key}" AND next LESS count)
            math(EXPR take "${i} + 2")
            list(SUBLIST segs 0 ${take} head)
            list(JOIN head "/" root)
            set(${out} "${root}" PARENT_SCOPE)
            return()
        endif()
        set(i ${next})
    endforeach()
endfunction()

# TRUE when any of the given files includes <bits/stdc++.h>
function(_dsa_uses_stdcpp_header out)
    foreach(f IN LISTS ARGN)
        file(STRINGS "${f}" hit REGEX "bits/stdc\\+\\+\\.h" LIMIT_COUNT 1)
        if(NOT "${hit}" STREQUAL "")
            set(${out} TRUE PARENT_SCOPE)
            return()
        endif()
    endforeach()
    set(${out} FALSE PARENT_SCOPE)
endfunction()

# Generates a tiny extra source file for the program: before main() it redirects stdin to
# "input_path" if that file exists (checked at run time, so adding the file needs no reconfigure).
function(_dsa_add_auto_input name input_path)
    string(REPLACE "\\" "\\\\" p "${input_path}")
    string(REPLACE "\"" "\\\"" p "${p}")
    set(template [=[
// Generated by CMake (DSA_AUTO_INPUT). Redirects stdin to the input file if it exists.
#include <cstdio>
namespace {
struct AutoInput {
    AutoInput() {
        const char* path = "@DSA_INPUT_PATH@";
        if (std::FILE* probe = std::fopen(path, "r")) {
            std::fclose(probe);
            if (std::freopen(path, "r", stdin) != nullptr) {
                std::fprintf(stderr, "[dsa] stdin <- %s\n", path);
            }
        }
    }
} auto_input;
}
]=])
    string(REPLACE "@DSA_INPUT_PATH@" "${p}" content "${template}")
    set(gen "${CMAKE_BINARY_DIR}/dsa_generated/${name}_autoinput.cpp")
    file(GENERATE OUTPUT "${gen}" CONTENT "${content}")
    target_sources(${name} PRIVATE "${gen}")
endfunction()

# Creates one executable with all the common settings applied.
function(_dsa_add_program name srcdir input_path)
    add_executable(${name} ${ARGN})
    target_link_libraries(${name} PRIVATE dsa_options)
    target_include_directories(${name} PRIVATE "${srcdir}")
    if(DSA_DEFINE_SOURCE_DIR)
        target_compile_definitions(${name} PRIVATE "DSA_SOURCE_DIR=\"${srcdir}\"")
    endif()
    if(_pch_possible)
        set(use_pch FALSE)
        if("${DSA_USE_PCH}" STREQUAL "AUTO")
            _dsa_uses_stdcpp_header(use_pch ${ARGN})
        elseif(DSA_USE_PCH)
            set(use_pch TRUE)
        endif()
        if(use_pch)
            _dsa_ensure_pch()
            target_precompile_headers(${name} REUSE_FROM dsa_pch)
        endif()
    endif()
    if(DSA_AUTO_INPUT)
        _dsa_add_auto_input(${name} "${input_path}")
    endif()
    if(NOT DSA_BUILD_ALL)
        set_target_properties(${name} PROPERTIES EXCLUDE_FROM_ALL ON)
    endif()
endfunction()

# Remembers one program to create later (names are resolved once all are known).
set(_n_targets 0)
set(_t_naming "")
set(_t_srcdir "")
set(_t_desc   "")
set(_t_input  "")
macro(_dsa_register naming srcdir desc input)
    list(APPEND _t_naming "${naming}")
    list(APPEND _t_srcdir "${srcdir}")
    list(APPEND _t_desc   "${desc}")
    list(APPEND _t_input  "${input}")
    set(_t_srcs_${_n_targets} ${ARGN})
    math(EXPR _n_targets "${_n_targets} + 1")
endmacro()

# -----------------------------------------------------------------------------
#  5. DISCOVERY
# -----------------------------------------------------------------------------
set(_root "${CMAKE_SOURCE_DIR}")

# Folders that never contain your code (build dirs, VCS, IDE, hidden folders ...).
set(_ignore_regex "(^|/)(cmake-build-[^/]*|build|out|CMakeFiles|_deps|[.][^/]*)/")
file(RELATIVE_PATH _bin_rel "${_root}" "${CMAKE_BINARY_DIR}")   # "../x" when build dir is outside

# 5a. Walk the tree ourselves instead of GLOB_RECURSE, so .git / build dirs are never
#     entered (GLOB_RECURSE cannot skip folders and re-scans everything on EVERY build).
#     A folder that has its own CMakeLists.txt is delegated and not scanned further.
set_property(GLOBAL PROPERTY _DSA_SOURCES "")
set_property(GLOBAL PROPERTY _DSA_DELEGATED "")

function(_dsa_walk reldir)
    if("${reldir}" STREQUAL "")
        set(abs "${_root}")
    else()
        set(abs "${_root}/${reldir}")
        if(EXISTS "${abs}/CMakeLists.txt")
            set_property(GLOBAL APPEND PROPERTY _DSA_DELEGATED "${reldir}")
            return()
        endif()
    endif()

    file(GLOB children CONFIGURE_DEPENDS LIST_DIRECTORIES true RELATIVE "${abs}" "${abs}/*")
    foreach(child IN LISTS children)
        if("${reldir}" STREQUAL "")
            set(child_rel "${child}")
        else()
            set(child_rel "${reldir}/${child}")
        endif()

        # CMake lists are ';'-separated, so a name containing ';' is torn in two by file(GLOB).
        if(NOT EXISTS "${_root}/${child_rel}")
            message(WARNING "DSA: ignoring '${child_rel}' - file/folder names containing ';' "
                    "are not supported by CMake. Please rename it.")
            continue()
        endif()
        if(NOT "${DSA_EXCLUDE_REGEX}" STREQUAL "" AND child_rel MATCHES "${DSA_EXCLUDE_REGEX}")
            continue()
        endif()

        if(IS_DIRECTORY "${_root}/${child_rel}")
            if("${child_rel}/" MATCHES "${_ignore_regex}")
                continue()
            endif()
            _dsa_is_under("${child_rel}" "${_bin_rel}" in_build_dir)   # build dir inside the tree
            if(in_build_dir AND NOT "${_bin_rel}" MATCHES "^\\.\\.")
                continue()
            endif()
            _dsa_walk("${child_rel}")
        elseif(child MATCHES "[.]([cC][pP][pP]|[cC][cC]|[cC][xX][xX])$")
            set_property(GLOBAL APPEND PROPERTY _DSA_SOURCES "${child_rel}")
        endif()
    endforeach()
endfunction()

_dsa_walk("")
get_property(_sources GLOBAL PROPERTY _DSA_SOURCES)
get_property(_claimed GLOBAL PROPERTY _DSA_DELEGATED)
list(SORT _sources)
list(SORT _claimed)

foreach(_d IN LISTS _claimed)
    add_subdirectory("${_root}/${_d}" "${CMAKE_BINARY_DIR}/subprojects/${_d}")
    if(DSA_VERBOSE)
        message(STATUS "DSA: '${_d}' has its own CMakeLists.txt -> delegated")
    endif()
endforeach()

# 5b. Group the sources: stand-alone programs vs. multi-file projects.
set(_proj_roots "")
set(_n_files 0)
foreach(_rel IN LISTS _sources)
    get_filename_component(_dir  "${_rel}" DIRECTORY)
    get_filename_component(_stem "${_rel}" NAME_WLE)
    _dsa_find_project_root("${_dir}" _proj)

    if(NOT "${_proj}" STREQUAL "")
        # Part of a multi-file project -> grouped, decided after the loop.
        list(FIND _proj_roots "${_proj}" _idx)
        if(_idx EQUAL -1)
            list(APPEND _proj_roots "${_proj}")
            list(LENGTH _proj_roots _cnt)
            math(EXPR _idx "${_cnt} - 1")
        endif()
        list(APPEND _proj_srcs_${_idx} "${_root}/${_rel}")
    else()
        # Stand-alone program.
        _dsa_has_main("${_root}/${_rel}" _has_main)
        if(DSA_REQUIRE_MAIN AND NOT _has_main)
            if(DSA_VERBOSE)
                message(STATUS "DSA: skipped (no main): ${_rel}")
            endif()
            continue()
        endif()
        if("${_dir}" STREQUAL "")
            set(_naming "${_stem}")
            set(_srcdir "${_root}")
        else()
            set(_naming "${_dir}/${_stem}")
            set(_srcdir "${_root}/${_dir}")
        endif()
        _dsa_register("${_naming}" "${_srcdir}" "${_rel}" "${_srcdir}/${_stem}.in" "${_root}/${_rel}")
        math(EXPR _n_files "${_n_files} + 1")
    endif()
endforeach()

# 5c. Multi-file projects: all their sources -> one executable.
set(_n_projects 0)
set(_p 0)
foreach(_proj IN LISTS _proj_roots)
    set(_srcs ${_proj_srcs_${_p}})
    set(_main_count 0)
    set(_main_files "")
    foreach(_s IN LISTS _srcs)
        _dsa_has_main("${_s}" _m)
        if(_m)
            math(EXPR _main_count "${_main_count} + 1")
            get_filename_component(_s_name "${_s}" NAME)
            list(APPEND _main_files "${_s_name}")
        endif()
    endforeach()
    list(LENGTH _srcs _src_count)
    if(DSA_REQUIRE_MAIN AND _main_count EQUAL 0)
        message(WARNING "DSA: project '${_proj}' has no main() - skipped.")
    else()
        if(_main_count GREATER 1)
            message(WARNING "DSA: project '${_proj}' has ${_main_count} files with main() "
                    "(${_main_files}) - the link will fail. A project may have only one.")
        endif()
        _dsa_register("${_proj}" "${_root}/${_proj}" "${_proj}/  (${_src_count} source file(s))"
                "${_root}/${_proj}/input.txt" ${_srcs})
        math(EXPR _n_projects "${_n_projects} + 1")
    endif()
    math(EXPR _p "${_p} + 1")
endforeach()

# -----------------------------------------------------------------------------
#  6. TARGET NAMES  (short & readable; path-qualified only if two names clash)
# -----------------------------------------------------------------------------
if(_n_targets GREATER 0)
    math(EXPR _last "${_n_targets} - 1")

    set(_shorts "")
    foreach(_i RANGE ${_last})
        list(GET _t_naming ${_i} _nm)
        get_filename_component(_leaf "${_nm}" NAME)
        _dsa_target_name("${_leaf}" _s)
        list(APPEND _shorts "${_s}")
    endforeach()

    set(_used "")
    foreach(_i RANGE ${_last})
        list(GET _t_naming ${_i} _nm)
        list(GET _shorts   ${_i} _s)
        string(TOLOWER "${_s}" _s_lower)

        set(_dups 0)
        foreach(_x IN LISTS _shorts)
            string(TOLOWER "${_x}" _x_lower)        # Windows: Vector.exe == vector.exe
            if("${_x_lower}" STREQUAL "${_s_lower}")
                math(EXPR _dups "${_dups} + 1")
            endif()
        endforeach()

        if(_dups GREATER 1)
            _dsa_target_name("${_nm}" _name)       # e.g. Array_Practise_Quest_Vector
        else()
            set(_name "${_s}")
        endif()

        set(_base "${_name}")
        set(_k 2)
        string(TOLOWER "${_name}" _name_lower)
        while(_name_lower IN_LIST _used OR TARGET "${_name}")
            set(_name "${_base}_${_k}")
            string(TOLOWER "${_name}" _name_lower)
            math(EXPR _k "${_k} + 1")
        endwhile()
        list(APPEND _used "${_name_lower}")

        list(GET _t_srcdir ${_i} _srcdir)
        list(GET _t_desc   ${_i} _desc)
        list(GET _t_input  ${_i} _input)
        _dsa_add_program("${_name}" "${_srcdir}" "${_input}" ${_t_srcs_${_i}})

        if(DSA_VERBOSE)
            message(STATUS "DSA: ${_name}   <-   ${_desc}")
        endif()
    endforeach()
endif()

message(STATUS "DSA Series: ${_n_targets} program(s) = ${_n_files} single-file + ${_n_projects} project(s) | "
        "C++${DSA_CXX_STANDARD} | ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION} | ${CMAKE_BUILD_TYPE}")