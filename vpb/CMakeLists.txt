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

file(GLOB vpb_src *.cpp)
file(GLOB vpb_inc *.h)

add_executable(bayonne-vpb ${vpb_src} ${vpb_inc})
set_source_dependencies(bayonne-vpb bayonne-runtime ccscript ccaudio2 ucommon)
target_link_libraries(bayonne-vpb bayonne-runtime ccscript ccaudio2 ucommon vpb ${UCOMMON_LIBRARIES} ${USES_SYSTEMD_LIBRARIES})

install(TARGETS bayonne-vpb DESTINATION ${CMAKE_INSTALL_SBINDIR})



