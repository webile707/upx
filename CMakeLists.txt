cmake_minimum_required(VERSION 3.8.0 FATAL_ERROR) # CMake >= 3.20 is recommended

# compilation config options
if(NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    # permissive config defaults when building from source code tarball
    option(UPX_CONFIG_DISABLE_GITREV   "Do not compile with default Git version info." ON)
    option(UPX_CONFIG_DISABLE_SANITIZE "Do not compile with default sanitize options." ON)
    option(UPX_CONFIG_DISABLE_WSTRICT  "Do not compile with strict compiler warnings." ON)
    option(UPX_CONFIG_DISABLE_WERROR   "Do not compile with default -Werror option."   ON)
else()
    # strict config defaults for Git developer builds
    message(STATUS "===== UPX info: strict config defaults enabled")
    option(UPX_CONFIG_DISABLE_GITREV   "Do not compile with default Git version info." OFF)
    option(UPX_CONFIG_DISABLE_SANITIZE "Do not compile with default sanitize options." OFF)
    option(UPX_CONFIG_DISABLE_WSTRICT  "Do not compile with strict compiler warnings." OFF)
    option(UPX_CONFIG_DISABLE_WERROR   "Do not compile with default -Werror option."   OFF)
endif()

# test config options (see below)
# NOTE: self-pack test can only work if the host executable format is supported by UPX!
option(UPX_CONFIG_DISABLE_SELF_PACK_TEST "Do not test packing UPX with itself" OFF)

#***********************************************************************
# init
#***********************************************************************

# Disallow in-source builds. Note that you will still have to manually
# clean up a few files if you accidentally try an in-source build.
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.git" AND NOT MSVC_IDE)
    set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
    set(CMAKE_DISABLE_SOURCE_CHANGES  ON)
    if(",${CMAKE_CURRENT_SOURCE_DIR}," STREQUAL ",${CMAKE_CURRENT_BINARY_DIR},")
        message(FATAL_ERROR "ERROR: In-source builds are not allowed, please use an extra build dir.")
    endif()
endif()
set(CMAKE_C_STANDARD_REQUIRED     ON)
set(CMAKE_CXX_STANDARD_REQUIRED   ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# determine Git revision
set(GITREV_SHORT "")
set(GITREV_PLUS "")
set(GIT_DESCRIBE "")
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.git" AND NOT UPX_CONFIG_DISABLE_GITREV)
    find_package(Git)
    if(Git_FOUND)
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --short=12 HEAD
            RESULT_VARIABLE result ERROR_QUIET
            OUTPUT_VARIABLE GITREV_SHORT OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        string(LENGTH "${GITREV_SHORT}" l)
        if(${result} EQUAL 0 AND ${l} EQUAL 12)
            execute_process(RESULT_VARIABLE result COMMAND "${GIT_EXECUTABLE}" diff --quiet)
            if(NOT ${result} EQUAL 0)
                set(GITREV_PLUS "+")
            endif()
        else()
            set(GITREV_SHORT "")
        endif()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" describe --match "v*.*.*" --tags --dirty
            RESULT_VARIABLE result ERROR_QUIET
            OUTPUT_VARIABLE GIT_DESCRIBE OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if(GIT_DESCRIBE MATCHES "^v?([0-9]+\\.[0-9]+\\.[0-9]+)-([0-9]+)-g(.+)$")
            set(GIT_DESCRIBE "${CMAKE_MATCH_1}-devel.${CMAKE_MATCH_2}+git-${CMAKE_MATCH_3}")
        endif()
    endif()
endif()
if(GITREV_SHORT)
    message(STATUS "UPX_VERSION_GITREV = \"${GITREV_SHORT}${GITREV_PLUS}\"")
    if(GIT_DESCRIBE)
        message(STATUS "UPX_VERSION_GIT_DESCRIBE = \"${GIT_DESCRIBE}\"")
    endif()
elseif(UPX_CONFIG_DISABLE_GITREV)
    message(STATUS "UPX_VERSION_GITREV: disabled")
else()
    message(STATUS "UPX_VERSION_GITREV: not set")
endif()

# CMake init
project(upx VERSION 4.0.2 LANGUAGES C CXX)

# set default build type to "Release"
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(is_multi_config)
    set(c "${CMAKE_CONFIGURATION_TYPES}")
    list(INSERT c 0 "Release")
    list(INSERT c 1 "Debug")
    if(CMAKE_BUILD_TYPE)
        list(INSERT c 0 "${CMAKE_BUILD_TYPE}")
    endif()
    list(REMOVE_DUPLICATES c)
    set(CMAKE_CONFIGURATION_TYPES "${c}" CACHE STRING "List of supported configuration types." FORCE)
elseif(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

#***********************************************************************
# targets
#***********************************************************************

set(UPX_CONFIG_DISABLE_THREADS ON) # multithreading is currently not used; maybe in UPX version 5
set(UPX_CONFIG_DISABLE_ZSTD ON) # zstd is currently not used; maybe in UPX version 5

if(NOT UPX_CONFIG_DISABLE_THREADS)
    find_package(Threads)
endif()

file(GLOB ucl_SOURCES "vendor/ucl/src/*.c")
list(SORT ucl_SOURCES)
add_library(upx_vendor_ucl STATIC ${ucl_SOURCES})
set_property(TARGET upx_vendor_ucl PROPERTY C_STANDARD 11)

file(GLOB zlib_SOURCES "vendor/zlib/*.c")
list(SORT zlib_SOURCES)
add_library(upx_vendor_zlib STATIC ${zlib_SOURCES})
set_property(TARGET upx_vendor_zlib PROPERTY C_STANDARD 11)

if(NOT UPX_CONFIG_DISABLE_ZSTD)
file(GLOB zstd_SOURCES "vendor/zstd/lib/*/*.c")
list(SORT zstd_SOURCES)
add_library(upx_vendor_zstd STATIC ${zstd_SOURCES})
set_property(TARGET upx_vendor_zstd PROPERTY C_STANDARD 11)
endif()

file(GLOB upx_SOURCES "src/*.cpp" "src/[cfu]*/*.cpp")
list(SORT upx_SOURCES)
add_executable(upx ${upx_SOURCES})
set_property(TARGET upx PROPERTY CXX_STANDARD 17)
target_link_libraries(upx upx_vendor_ucl upx_vendor_zlib)
if(NOT UPX_CONFIG_DISABLE_ZSTD)
    target_link_libraries(upx upx_vendor_zstd)
endif()
if(Threads_FOUND)
    target_link_libraries(upx Threads::Threads)
endif()

#***********************************************************************
# compilation flags
#***********************************************************************

if(UPX_CONFIG_DISABLE_WSTRICT)
    # enable all basic warnings
    set(warn_Wall -Wall)
    set(warn_WN -W3)
else()
    # enable lots of strict warnings
    set(warn_Wall -Wall -Wextra -Wcast-align -Wcast-qual -Wmissing-declarations -Wpointer-arith -Wshadow -Wvla -Wwrite-strings)
    set(warn_WN -W4)
endif()
if(UPX_CONFIG_DISABLE_WERROR)
    set(warn_Werror "")
    set(warn_WX "")
else()
    set(warn_Werror -Werror)
    set(warn_WX -WX)
endif()

if(NOT MSVC)
    # use -O2 instead of -O3 to reduce code size
    string(REGEX REPLACE "(^| )-O3( |$$)" "\\1-O2\\2" a "${CMAKE_C_FLAGS_RELEASE}")
    string(REGEX REPLACE "(^| )-O3( |$$)" "\\1-O2\\2" b "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_C_FLAGS_RELEASE "${a}" CACHE STRING "Flags used by the C compiler during RELEASE builds." FORCE)
    set(CMAKE_CXX_FLAGS_RELEASE "${b}" CACHE STRING "Flags used by the CXX compiler during RELEASE builds." FORCE)
endif()

if(MSVC)
    # disable silly warnings about using "deprecated" POSIX functions like fopen()
    add_definitions(-D_CRT_NONSTDC_NO_DEPRECATE)
    add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
    add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    # set __cplusplus according to selected C++ standard; use new preprocessor
    add_definitions(-Zc:__cplusplus -Zc:preprocessor)
else()
    # protect against security threats caused by misguided compiler "optimizations"
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_definitions(-fno-delete-null-pointer-checks -fno-lifetime-dse)
    endif()
    add_definitions(-fno-strict-aliasing -fno-strict-overflow -funsigned-char)
    # disable overambitious auto-vectorization until this actually gains something
    add_definitions(-fno-tree-vectorize)
endif()

# compile a target with -O2 even in Debug build
function(upx_compile_target_debug_with_O2 t)
    if(MSVC)
        # msvc uses some Debug compile options like -RTC1 that are incompatible with -O2
    else()
        target_compile_options(${t} PRIVATE $<$<CONFIG:Debug>:-O2>)
    endif()
endfunction()

function(upx_sanitize_target t)
    if(NOT UPX_CONFIG_DISABLE_SANITIZE)
        if(MSVC)
            # msvc uses -GS (similar to -fstack-protector) by default
        elseif(CMAKE_C_PLATFORM_ID MATCHES "^MinGW" OR MINGW OR CYGWIN)
            # avoid link errors with current MinGW-w64 versions
            # see https://www.mingw-w64.org/contribute/#sanitizers-asan-tsan-usan
        else()
            # default sanitizer for Debug builds
            target_compile_options(${t} PRIVATE $<$<CONFIG:Debug>:-fsanitize=undefined -fsanitize-undefined-trap-on-error -fstack-protector-all>)
            # default sanitizer for Release builds
            target_compile_options(${t} PRIVATE $<$<CONFIG:Release>:-fstack-protector>)
        endif()
    endif()
endfunction()

set(t upx_vendor_ucl)
target_include_directories(${t} PRIVATE vendor/ucl/include vendor/ucl)
upx_compile_target_debug_with_O2(${t})
upx_sanitize_target(${t})
if(MSVC)
    target_compile_options(${t} PRIVATE -J ${warn_WN} ${warn_WX})
else()
    target_compile_options(${t} PRIVATE ${warn_Wall} ${warn_Werror})
endif()

set(t upx_vendor_zlib)
upx_compile_target_debug_with_O2(${t})
upx_sanitize_target(${t})
target_compile_definitions(${t} PRIVATE HAVE_STDARG_H=1 HAVE_VSNPRINTF=1)
if(MSVC)
    target_compile_options(${t} PRIVATE -J -W3 ${warn_WX})
else()
    target_compile_definitions(${t} PRIVATE HAVE_UNISTD_H=1)
    # clang-15: -Wno-strict-prototypes is needed to silence the new -Wdeprecated-non-prototype warning
    target_compile_options(${t} PRIVATE -Wall -Wno-strict-prototypes ${warn_Werror})
    ##target_compile_options(${t} PRIVATE ${warn_Wall} -Wno-cast-align -Wno-cast-qual -Wno-strict-prototypes ${warn_Werror})
endif()

if(NOT UPX_CONFIG_DISABLE_ZSTD)
set(t upx_vendor_zstd)
upx_compile_target_debug_with_O2(${t})
upx_sanitize_target(${t})
target_compile_definitions(${t} PRIVATE DYNAMIC_BMI2=0 ZSTD_DISABLE_ASM=1)
if(MSVC)
    target_compile_options(${t} PRIVATE -J ${warn_WN} ${warn_WX})
else()
    target_compile_options(${t} PRIVATE ${warn_Wall} ${warn_Werror})
endif()
endif()

set(t upx)
target_include_directories(${t} PRIVATE vendor)
target_compile_definitions(${t} PRIVATE $<$<CONFIG:Debug>:DEBUG=1>)
if(GITREV_SHORT)
    target_compile_definitions(${t} PRIVATE UPX_VERSION_GITREV="${GITREV_SHORT}${GITREV_PLUS}")
    if(GIT_DESCRIBE)
        target_compile_definitions(${t} PRIVATE UPX_VERSION_GIT_DESCRIBE="${GIT_DESCRIBE}")
    endif()
endif()
if(Threads_FOUND)
    target_compile_definitions(${t} PRIVATE WITH_THREADS=1)
endif()
if(UPX_CONFIG_DISABLE_WSTRICT)
    target_compile_definitions(${t} PRIVATE UPX_CONFIG_DISABLE_WSTRICT=1)
endif()
if(UPX_CONFIG_DISABLE_WERROR)
    target_compile_definitions(${t} PRIVATE UPX_CONFIG_DISABLE_WERROR=1)
endif()
if(NOT UPX_CONFIG_DISABLE_ZSTD)
    target_compile_definitions(${t} PRIVATE WITH_ZSTD=1)
endif()
#upx_compile_target_debug_with_O2(${t})
upx_sanitize_target(${t})
if(MSVC)
    target_compile_options(${t} PRIVATE -EHsc -J ${warn_WN} ${warn_WX})
else()
    target_compile_options(${t} PRIVATE ${warn_Wall} ${warn_Werror})
endif()

#***********************************************************************
# ctest
# make test
# ninja test
#***********************************************************************

if(NOT UPX_CONFIG_CMAKE_DISABLE_TEST)

include(CTest)
if(NOT CMAKE_CROSSCOMPILING)
    add_test(NAME upx-version COMMAND upx --version)
    add_test(NAME upx-help    COMMAND upx --help)
endif()
if(NOT CMAKE_CROSSCOMPILING AND NOT UPX_CONFIG_DISABLE_SELF_PACK_TEST)
    # NOTE: these tests can only work if the host executable format is supported by UPX!
    function(upx_add_test)
        set(name ${ARGV0})
        list(REMOVE_AT ARGV 0)
        add_test(NAME ${name} COMMAND ${ARGV})
        set_tests_properties(${name} PROPERTIES RUN_SERIAL TRUE) # run these tests sequentially
    endfunction()
    set(exe ${CMAKE_EXECUTABLE_SUFFIX})
    set(upx_self_exe "$<TARGET_FILE:upx>")
    set(fo "--force-overwrite")
    upx_add_test(upx-self-pack      upx -3         ${upx_self_exe} ${fo} -o upx-packed${exe})
    upx_add_test(upx-self-pack-n2b  upx -3 --nrv2b ${upx_self_exe} ${fo} -o upx-packed-n2b${exe})
    upx_add_test(upx-self-pack-n2d  upx -3 --nrv2d ${upx_self_exe} ${fo} -o upx-packed-n2d${exe})
    upx_add_test(upx-self-pack-n2e  upx -3 --nrv2e ${upx_self_exe} ${fo} -o upx-packed-n2e${exe})
    upx_add_test(upx-self-pack-lzma upx -3 --lzma  ${upx_self_exe} ${fo} -o upx-packed-lzma${exe})
    upx_add_test(upx-list           upx -l         upx-packed${exe} upx-packed-n2b${exe} upx-packed-n2d${exe} upx-packed-n2e${exe} upx-packed-lzma${exe})
    upx_add_test(upx-fileinfo       upx --fileinfo upx-packed${exe} upx-packed-n2b${exe} upx-packed-n2d${exe} upx-packed-n2e${exe} upx-packed-lzma${exe})
    upx_add_test(upx-test           upx -t         upx-packed${exe} upx-packed-n2b${exe} upx-packed-n2d${exe} upx-packed-n2e${exe} upx-packed-lzma${exe})
    upx_add_test(upx-unpack         upx -d upx-packed${exe} ${fo} -o upx-unpacked${exe})
    upx_add_test(upx-run-unpacked   ./upx-unpacked${exe} --version-short)
    upx_add_test(upx-run-packed     ./upx-packed${exe} --version-short)
endif()

endif() # UPX_CONFIG_CMAKE_DISABLE_TEST

#***********************************************************************
# cmake --install .
# make install
# ninja install
#***********************************************************************

if(NOT UPX_CONFIG_CMAKE_DISABLE_INSTALL)

# installation prefix and directories
if(NOT CMAKE_INSTALL_PREFIX)
    #message(FATAL_ERROR "ERROR: CMAKE_INSTALL_PREFIX is not defined.")
    message(WARNING "WARNING: CMAKE_INSTALL_PREFIX is not defined.")
endif()
if(CMAKE_INSTALL_PREFIX)
    include(GNUInstallDirs)
endif()
if(CMAKE_INSTALL_PREFIX AND DEFINED CMAKE_INSTALL_FULL_BINDIR)
    install(TARGETS upx DESTINATION "${CMAKE_INSTALL_FULL_BINDIR}")
    install(FILES
        COPYING LICENSE NEWS README doc/THANKS.txt doc/upx-doc.html doc/upx-doc.txt
        DESTINATION "${CMAKE_INSTALL_FULL_DOCDIR}"
    )
    install(FILES doc/upx.1 DESTINATION "${CMAKE_INSTALL_FULL_MANDIR}/man1")
endif()

endif() # UPX_CONFIG_CMAKE_DISABLE_INSTALL

#***********************************************************************
# finally print some info about the build configuration
#***********************************************************************

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/maint/make/CMakeLists.maint.txt")
include("${CMAKE_CURRENT_SOURCE_DIR}/maint/make/CMakeLists.maint.txt")
endif()

function(print_var)
    foreach(v ${ARGV})
        if(${v})
            message(STATUS "${v} = ${${v}}")
        endif()
    endforeach()
endfunction()
if(NOT UPX_CONFIG_CMAKE_DISABLE_PRINT_INFO)
print_var(CMAKE_HOST_SYSTEM_NAME CMAKE_HOST_SYSTEM_VERSION)
print_var(CMAKE_SYSTEM_NAME CMAKE_SYSTEM_VERSION CMAKE_CROSSCOMPILING)
print_var(CMAKE_C_COMPILER_ID CMAKE_C_COMPILER_VERSION CMAKE_C_COMPILER_ARCHITECTURE_ID CMAKE_C_PLATFORM_ID CMAKE_C_COMPILER_ABI)
print_var(CMAKE_CXX_COMPILER_ID CMAKE_CXX_COMPILER_VERSION CMAKE_CXX_COMPILER_ARCHITECTURE_ID CMAKE_CXX_PLATFORM_ID CMAKE_CXX_COMPILER_ABI)
endif() # UPX_CONFIG_CMAKE_DISABLE_PRINT_INFO
print_var(CMAKE_INSTALL_PREFIX CMAKE_CONFIGURATION_TYPES CMAKE_BUILD_TYPE)
if(CMAKE_BUILD_TYPE AND NOT CMAKE_BUILD_TYPE MATCHES "^(Debug|None|Release)$")
    message(WARNING "WARNING: unsupported CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}; please use \"Debug\" or \"Release\"")
endif()

# extra sanity checks to detect incompatible C vs CXX settings
if(NOT UPX_CONFIG_CMAKE_DISABLE_PLATFORM_CHECK)
if(NOT ",${CMAKE_C_PLATFORM_ID}," STREQUAL ",${CMAKE_CXX_PLATFORM_ID},")
    message(FATAL_ERROR "ERROR: CMAKE_C_PLATFORM_ID CMAKE_CXX_PLATFORM_ID mismatch")
endif()
if(NOT ",${CMAKE_C_COMPILER_ABI}," STREQUAL ",${CMAKE_CXX_COMPILER_ABI},")
    message(FATAL_ERROR "ERROR: CMAKE_C_COMPILER_ABI CMAKE_CXX_COMPILER_ABI mismatch")
endif()
endif() # UPX_CONFIG_CMAKE_DISABLE_PLATFORM_CHECK

# vim:set ft=cmake ts=4 sw=4 tw=0 et:
