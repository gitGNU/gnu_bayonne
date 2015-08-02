# Copyright (C) 2010 David Sugar, Tycho Softworks
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#

cmake_minimum_required(VERSION 2.6)
PROJECT(bayonne)
set (VERSION 3.0.0)
set (USES_UCOMMON_REQUIRED 6.4.4)
set (USES_CCSCRIPT_REQUIRED 5.0.0)
set (USES_CCAUDIO2_REQUIRED 2.1.0)

INCLUDE(CheckIncludeFiles)
INCLUDE(CheckIncludeFileCXX)
INCLUDE(CheckFunctionExists)
INCLUDE(GNUInstallDirs)

# when we override default install prefix, assume full path is used...

set(DEFAULT_PAGING 4096)

if (WIN32)
    option(BUILD_STATIC "Set to ON to build shared libraries" ON)
    option(BUILD_RUNTIME "Set to OFF to build static runtime" ON)
    if(BUILD_RUNTIME)
        set(BUILD_RUNTIME_TYPE SHARED)
    endif()
    set(HAVE_SIGWAIT false)
    set(HAVE_SIGWAIT2 off CACHE BOOL "disable sigwait on windows" FORCE)
    MARK_AS_ADVANCED(FORCE HAVE_SIGWAIT2)
else()
    option(BUILD_STATIC "Set to ON to build static libraries" OFF)
    option(HAVE_SIGWAIT2 "Set to OFF to build with single argument" ON)
    set(HAVE_SIGWAIT true)
endif()

MESSAGE( STATUS "Configuring GNU bayonne ${VERSION}...")

if (USES_UCOMMON_INCLUDE_DIRS)
    message(STATUS "  Using local ucommon dependency")
else()
    find_package(PkgConfig)
    pkg_check_modules(USES_UCOMMON REQUIRED ucommon>=${USES_UCOMMON_REQUIRED})
endif()

if (USES_CCSCRIPT_INCLUDE_DIRS)
    message(STATUS "  Using local ccscript dependency")
else()
    find_package(PkgConfig)
    pkg_check_modules(USES_CCSCRIPT REQUIRED ccscript>=${USES_CCSCRIPT_REQUIRED})
endif()

if (USES_CCAUDIO2_INCLUDE_DIRS)
    message(STATUS "  Using local ccaudio2 dependency")
else()
    find_package(PkgConfig)
    pkg_check_modules(USES_CCAUDIO2 REQUIRED ccaudio2>=${USES_CCAUDIO2_REQUIRED})
endif()

include_directories(${USES_CCSCRIPT_INCLUDE_DIRS} ${USES_CCAUDIO2_INCLUDE_DIRS} ${USES_UCOMMON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/inc ${WITH_INCLUDES})
link_directories(${USES_CCSCRIPT_LIBRARY_DIRS} ${USES_CCAUDIO2_LIBRARY_DIRS} ${USES_UCOMMON_LIBRARY_DIRS} ${WITH_LIBS})
add_definitions(${USES_CCSCRIPT_CFLAGS} ${USES_CCAUDIO2_CFLAGS} ${USES_UCOMMON_CFLAGS} ${WITH_CFLAGS})

if(USES_UCOMMON_CMAKE_MACROS)
	set(CMAKE_MODULE_PATH "${USES_UCOMMON_CMAKE_MACROS}")
elseif(EXISTS "${USES_UCOMMON_PREFIX}/share/ucommon/cmake")
	set(CMAKE_MODULE_PATH "${USES_UCOMMON_PREFIX}/share/ucommon/cmake")
else()
	set(CMAKE_MODULE_PATH "${CMAKE_INSTALL_FULL_DATADIR}/ucommon/cmake")
endif()

include(CapeConfig)
include(CapeMakeTargets)

if(BUILD_STATIC AND NOT BUILD_SHARED)
    set(BUILD_LIBRARY_TYPE STATIC)
else()
    set(BUILD_LIBRARY_TYPE SHARED)
endif()

if(NOT BUILD_RUNTIME_TYPE)
    set(BUILD_RUNTIME_TYPE ${BUILD_LIBRARY_TYPE})
endif()

check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(pwd.h HAVE_PWD_H)
check_include_files(eXosip2/eXosip.h HAVE_EXOSIP2)
check_include_file_cxx(vpbapi.h HAVE_VPBAPI)
check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(setpgrp HAVE_SETPGRP)
check_function_exists(getuid HAVE_GETUID)
check_function_exists(mkfifo HAVE_MKFIFO)

pkg_check_modules(USES_SYSTEMD libsystemd-daemon>=44)
if(USES_SYSTEMD_FOUND)
    set(HAVE_SYSTEMD true)
endif()

if(HAVE_EXOSIP2)
    check_include_files(openssl/ssl.h HAVE_OPENSSL)
endif()

if(HAVE_OPENSSL)
    if(WIN32 AND NOT CYGWIN AND NOT MINGW)
        set (EXOSIP2_LIBS eXosip2 osip2 osipparser2 ssleay32 libeay32 gdi32)
    else()
        set (EXOSIP2_LIBS eXosip2 osip2 osipparser2 ssl crypto z)
    endif()
elseif(HAVE_EXOSIP2)
    set(EXOSIP2_LIBS eXosip2 osip2 osipparser2)
endif()

check_include_files(resolv.h HAVE_RESOLV_H)
if(HAVE_RESOLV_H)
    set(USES_UCOMMON_LIBRARIES ${USES_UCOMMON_LIBRARIES} resolv)
endif()

if(HAVE_EXOSIP2 AND WIN32 AND NOT CYGWIN)
    set(EXOSIP2_LIBS ${EXOSIP2_LIBS} iphlpapi dnsapi)
endif()

file(GLOB runtime_src common/*.cpp)
file(GLOB runtime_inc inc/bayonne/*.h)
file(GLOB common_inc common/*.h)

pc_libs(PKG_BAYONNE_LIBS ${USES_CCSCRIPT_LIBRARIES} ${USES_CCAUDIO2_LIBRARIES} ${USES_UCOMMON_LIBRARIES} ${USES_SYSTEMD_LIBRARIES})

create_headers()
create_specfile()
create_pcfiles(pc_files libbayonne)
create_scripts(noinst_scr PKGBUILD)

add_library(bayonne-runtime ${BUILD_RUNTIME_TYPE} ${runtime_src} ${runtime_inc} ${common_inc})
set_target_properties(bayonne-runtime PROPERTIES OUTPUT_NAME bayonne)
set_library_version(bayonne-runtime)
set_source_dependencies(bayonne-runtime ccscript usecure ucommon)
target_link_libraries(bayonne-runtime ccscript ccaudio2 usecure ucommon ${USES_CCSCRIPT_LIBRARIES} ${USES_CCAUDIO2_LIBRARIES} ${SECURE_LIBRARIES} ${USES_UCOMMON_LIBRARIES} ${USES_SYSTEMD_LIBRARIES})

if(HAVE_EXOSIP2)
    add_subdirectory(sipw)
endif()

if(HAVE_VPBAPI)
    add_subdirectory(vpb)
endif()

add_executable(bayonne-control utils/baycontrol.cpp)
set_source_dependencies(bayonne-control ucommon)
target_link_libraries(bayonne-control ucommon ${USES_UCOMMON_LIBRARIES})
set_target_properties(bayonne-control PROPERTIES OUTPUT_NAME baycontrol)

add_executable(bayonne-lint utils/baylint.cpp)
set_source_dependencies(bayonne-lint bayonne-runtime ucommon ccscript)
target_link_libraries(bayonne-lint bayonne-runtime ucommon ${USES_UCOMMON_LIBRARIES})
set_target_properties(bayonne-lint PROPERTIES OUTPUT_NAME baylint)

install(TARGETS bayonne-runtime DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(TARGETS bayonne-control bayonne-lint DESTINATION ${CMAKE_INSTALL_BINDIR})

add_make_lint_target()
add_make_uninstall_target()
