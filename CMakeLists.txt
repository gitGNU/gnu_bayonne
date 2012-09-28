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
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckFunctionExists)
INCLUDE(CheckLibraryExists)

set (VERSION 2.99.1)

set(RC_VERSION ${VERSION})
STRING(REGEX REPLACE "[.]" "," RC_VERSION ${RC_VERSION})
set(RC_VERSION "${RC_VERSION},0")

set(SOVERSION ${VERSION})
STRING(REGEX REPLACE "[.].*$" "" SOVERSION ${SOVERSION})

# when we override default install prefix, assume full path is used...

set(INSTALL_INCLUDEDIR include)
set(INSTALL_BINDIR bin)
set(INSTALL_SBINDIR sbin)

if(WIN32)
    set(INSTALL_MANDIR man)
    set(INSTALL_DOCDIR doc/${PROJECT_NAME})
    set(INSTALL_LOCALE locale)
    set(INSTALL_LIBDIR bin)
    set(INSTALL_INFODIR info)
    set(SYSCONFDIR "etc" CACHE PATH "config directory")
    set(STATEDIR "var" CACHE PATH "state directory")
else()
    set(INSTALL_MANDIR share/man)
    set(INSTALL_LOCALE share/locale)
    set(INSTALL_DOCDIR share/doc/${PROJECT_NAME})
    set(INSTALL_INFODIR share/info)
    set(INSTALL_LIBDIR lib${LIB_POSTFIX})
    set(SYSCONFDIR "/etc" CACHE PATH "config directory")
    set(STATEDIR "/var" CACHE PATH "state directory")
endif()

if(WIN32)
    set(BAYONNE_CFGPATH "${CMAKE_INSTALL_PREFIX}/${SYSCONFDIR}")
    set(BAYONNE_LIBEXEC ${CMAKE_INSTALL_PREFIX}/libexec)
    set(DEFAULT_VARPATH "${CMAKE_INSTALL_PREFIX}/${STATEDIR}")
else()
    set(BAYONNE_CFGPATH ${SYSCONFDIR}/bayonne)
    set(BAYONNE_LIBEXEC ${CMAKE_INSTALL_PREFIX}/libexec/bayonne)
    set(DEFAULT_VARPATH ${STATEDIR})
endif()

set(DEFAULT_DATADIR ${CMAKE_INSTALL_PREFIX}/${INSTALL_DATADIR})
set(DEFAULT_PAGING 4096)
set(DEFAULT_LIBPATH ${CMAKE_INSTALL_PREFIX}/${INSTALL_LIBDIR})

if (WIN32)
    set(BUILD_STATIC ON CACHE BOOL "static linking only" FORCE)
    MARK_AS_ADVANCED(FORCE BUILD_STATIC)
    # too broken to build dll's with const static members...
    if(NOT MSVC60)
        option(BUILD_RUNTIME "Set to OFF to build static runtime" ON)
        if(BUILD_RUNTIME)
            set(BUILD_RUNTIME_TYPE SHARED)
        endif()
    endif()
    set(HAVE_SIGWAIT false)
    set(HAVE_SIGWAIT2 off CACHE BOOL "disable sigwait on windows" FORCE)
    MARK_AS_ADVANCED(FORCE HAVE_SIGWAIT2)
    option(EXOSIP2_TLS "Set to ON to check eXosip2 TLS support" OFF)
else()
    option(BUILD_STATIC "Set to ON to build static libraries" OFF)
    option(HAVE_SIGWAIT2 "Set to OFF to build with single argument" ON)
    set(HAVE_SIGWAIT true)
    option(EXOSIP2_TLS "Set to OFF to disable eXosip2 TLS support check" ON)
endif()

option(BUILD_TESTING "Set to ON to build test programs" OFF)
MESSAGE( STATUS "Configuring GNU bayonne ${VERSION}...")

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_NAME} server")
SET(CPACK_PACKAGE_VENDOR              "David Sugar")
SET(CPACK_PACKAGE_DESCRIPTION_FILE    "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
set(PACKAGE_FILE_NAME                 ${PROJECT_NAME})
set(PACKAGE_FILE_VERSION              ${VERSION})

IF (WIN32)
    SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}-${PACKAGE_FILE_VERSION}")
ELSE (WIN32)
    SET(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}-bin-${PACKAGE_FILE_VERSION}")
    if (NOT CPACK_GENERATOR)
        SET(CPACK_GENERATOR "TBZ2")
    endif()
ENDIF (WIN32)

IF(WIN32 AND NOT UNIX)
    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_FILE_NAME}")

    # There is a bug in NSI that does not handle full unix paths properly. Make
    # sure there is at least one set of four (4) backlasshes.
    # SET(CPACK_PACKAGE_ICON "${CMake_SOURCE_DIR}/Utilities/Release\\\\InstallIcon.bmp")
    # SET(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\MyExecutable.exe")
    SET(CPACK_NSIS_DISPLAY_NAME "${CPACK_PACKAGE_INSTALL_DIRECTORY} ${PROJECT_NAME}$")
    SET(CPACK_NSIS_HELP_LINK "http:\\\\\\\\www.gnutelephony.org")
    SET(CPACK_NSIS_URL_INFO_ABOUT "http:\\\\\\\\www.gnutelephony.org")
    SET(CPACK_NSIS_CONTACT "dyfet@gnutelephony.org")
    SET(CPACK_NSIS_MODIFY_PATH ON)

ENDIF(WIN32 AND NOT UNIX)

set(CPACK_COMPONENTS_ALL server)
set(CPACK_COMPONENT_SERVER_DISPLAY_NAME "Server")
set(CPACK_COMPONENT_SERVER_DESCRIPTION  "Server and Runtime")
SET(CPACK_SOURCE_IGNORE_FILES ".bzr" "build" ".swp$" ".*~" ".svn" ".git")
SET(CPACK_SOURCE_GENERATOR "TBZ2")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}-${PACKAGE_FILE_VERSION}")

# This must always be last!
INCLUDE(CPack)

# set to true for debug and trace during CMakeLists development
set(CMAKE_VERBOSE_MAKEFILE FALSE)

# Set defaults and pass common options.  Common build options can be passed
# to cmake using cmake -DWITH_CFLAGS="...", WITH_LIBS, and WITH_INCLUDES
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc ${WITH_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR}/server)
add_definitions(${WITH_CFLAGS})
link_libraries(${WITH_LIBS})

if (USES_UCOMMON_INCLUDE_DIRS)
    message(STATUS "  Using local ucommon dependency")
else()
    find_package(PkgConfig)
    pkg_check_modules(USES_UCOMMON REQUIRED ucommon>=4.0.1)
endif()

include_directories(${USES_UCOMMON_INCLUDE_DIRS})
link_directories(${USES_UCOMMON_LIBRARY_DIRS})
add_definitions(${USES_UCOMMON_CFLAGS})

# by default we build static libs for windows, shared libs for unix.
# we may also set this from a top level cmake or -DWITH_XX_LIBS

if(BUILD_STATIC AND NOT BUILD_SHARED)
    set(BUILD_LIBRARY_TYPE STATIC)
else()
    set(BUILD_LIBRARY_TYPE SHARED)
endif()

if(NOT BUILD_RUNTIME_TYPE)
    set(BUILD_RUNTIME_TYPE ${BUILD_LIBRARY_TYPE})
endif()

check_include_files(sys/inotify.h HAVE_SYS_INOTIFY_H)
check_include_files(sys/resource.h HAVE_SYS_RESOURCE_H)
check_include_files(pwd.h HAVE_PWD_H)
check_include_files(speex/speex.h HAVE_SPEEX_SPEEX_H)
check_include_files(gsm.h HAVE_GSM_H)
check_include_files(gsm/gsm.h HAVE_GSM_GSM_H)
check_include_files(endian.h HAVE_ENDIAN_H)
check_include_files(math.h HAVE_MATH_H)
check_include_files(stdint.h HAVE_STDINT_H)
check_include_files(termios.h HAVE_TERMIOS_H)
check_include_files(ioctl.h HAVE_IOCTL_H)
check_include_files(sys/ioctl.h HAVE_SYS_IOCTL_H)
check_include_files(sys/poll.h HAVE_SYS_POLL_H)
check_include_files(sys/uio.h HAVE_SYS_UIO_H)
check_include_files(eXosip2/eXosip.h HAVE_EXOSIP2)
check_include_files(srtp/srtp.h HAVE_SRTP)
#check_include_files(vpbapi.h HAVE_VPBAPI)
#check_include_files(capi20.h HAVE_CAPI20)
#check_include_files(sys/telephony.h HAVE_SYS_TELEPHONY_H)
#check_include_files(linux/telephony.h HAVE_LINUX_TELEPHONY_H)
#check_include_files(sys/ixjuser.h HAVE_SYS_IXJUSER_H)
#check_include_files(linux/ixjuser.h HAVE_LINUX_IXJUSER_H)
check_function_exists(setrlimit HAVE_SETRLIMIT)
check_function_exists(setpgrp HAVE_SETPGRP)
check_function_exists(getuid HAVE_GETUID)
check_function_exists(mkfifo HAVE_MKFIFO)

check_library_exists(m sqrt "" MATH_LIB)
if(MATH_LIB)
    set(EXPORT_LIBS ${EXPORT_LIBS} m)
endif()

if(WIN32 AND NOT MINGW AND NOT MSYS AND NOT CYGWIN)
    if(HAVE_SPEEX_SPEEX_H)
        set(EXPORT_LIBS ${EXPORT_LIBS} libspeex)
    endif()

    if(HAVE_GSM_H OR HAVE_GSM_GSM_H)
        set(EXPORT_LIBS ${EXPORT_LIBS} libgsm)
    endif()
else()
    if(HAVE_SPEEX_SPEEX_H)
        set(EXPORT_LIBS ${EXPORT_LIBS} speex)
    endif()

    if(HAVE_GSM_H OR HAVE_GSM_GSM_H)
        set(EXPORT_LIBS ${EXPORT_LIBS} gsm)
    endif()
endif()

if(NOT HAVE_EXOSIP2)
    set(EXOSIP2_TLS OFF CACHE BOOL "no tls if no exosip2" FORCE)
    MARK_AS_ADVANCED(EXOSIP2_TLS)
endif()

if(EXOSIP2_TLS AND HAVE_EXOSIP2)
    check_include_files(openssl/openssl.h HAVE_TLS)
endif()

if(HAVE_TLS)
    if(WIN32 AND NOT CYGWIN AND NOT MINGW)
        set (EXOSIP2_LIBS eXosip2 osip2 osipparser2 ssleay32 libeay32 gdi32)
    else()
        set (EXOSIP2_LIBS eXosip2 osip2 osipparser2 ssl crypto z)
    endif()
elseif(HAVE_EXOSIP2)
    set(EXOSIP2_LIBS eXosip2 osip2 osipparser2)
endif()

check_include_files(resolv.h HAVE_RESOLV)
if(HAVE_RESOLV)
    set(EXOSIP2_LIBS ${EXOSIP2_LIBS} resolv)
endif()

if(HAVE_EXOSIP2 AND WIN32 AND NOT CYGWIN)
    set(EXOSIP2_LIBS ${EXOSIP2_LIBS} iphlpapi dnsapi)
endif()

configure_file(bayonne-config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/bayonne-config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB runtime_src runtime/*.cpp)
file(GLOB runtime_inc inc/bayonne.h)

add_library(bayonne-runtime ${BUILD_RUNTIME_TYPE} ${runtime_src} ${runtime_inc})
set_target_properties(bayonne-runtime PROPERTIES OUTPUT_NAME bayonne)
add_dependencies(bayonne-runtime ucommon)
target_link_libraries(bayonne-runtime ${EXPORT_LIBS} ${USES_UCOMMON_LIBRARIES})

add_subdirectory(server)
add_subdirectory(utils)

if(BUILD_TESTING)
    ENABLE_TESTING()
    add_subdirectory(test)
endif()

if(HAVE_EXOSIP2)
    add_subdirectory(sipwitch)
endif()

install(TARGETS bayonne-runtime DESTINATION ${INSTALL_LIBDIR})


