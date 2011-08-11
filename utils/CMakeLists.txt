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
add_executable(phrasebook phrasebook.cpp)
add_dependencies(phrasebook bayonne-runtime ucommon)
target_link_libraries(phrasebook bayonne-runtime ${USES_UCOMMON_LIBRARIES})

add_executable(tonetool tonetool.cpp)
add_dependencies(tonetool bayonne-runtime ucommon)
target_link_libraries(tonetool bayonne-runtime ${USES_UCOMMON_LIBRARIES})

add_executable(baylint baylint.cpp)
add_dependencies(baylint bayonne-runtime ucommon)
target_link_libraries(baylint bayonne-runtime ${USES_UCOMMON_LIBRARIES})

install(TARGETS phrasebook tonetool baylint DESTINATION ${INSTALL_BINDIR})


