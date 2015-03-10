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

file(GLOB sipw_src *.cpp)
file(GLOB sipw_inc *.h)

add_executable(bayonne-sipw ${sipw_src} ${sipw_inc})
set_source_dependencies(bayonne-sipw bayonne-runtime ccscript ccaudio2 usecure ucommon)
target_link_libraries(bayonne-sipw bayonne-runtime ccscript ccaudio2 usecure ucommon ${EXOSIP2_LIBS} ${SECURE_LIBRARIES} ${UCOMMON_LIBRARIES} ${USES_SYSTEMD_LIBRARIES})

install(TARGETS bayonne-sipw DESTINATION ${CMAKE_INSTALL_SBINDIR})



