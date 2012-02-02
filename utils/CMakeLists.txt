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
add_executable(bayonne-phrasebook phrasebook.cpp)
add_dependencies(bayonne-phrasebook bayonne-runtime ucommon)
target_link_libraries(bayonne-phrasebook bayonne-runtime ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})
set_target_properties(bayonne-phrasebook PROPERTIES OUTPUT_NAME phrasebook)

add_executable(bayonne-tonetool tonetool.cpp)
add_dependencies(bayonne-tonetool bayonne-runtime ucommon)
target_link_libraries(bayonne-tonetool bayonne-runtime ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})
set_target_properties(bayonne-tonetool PROPERTIES OUTPUT_NAME tonetool)

add_executable(bayonne-audiotool audiotool.cpp)
add_dependencies(bayonne-audiotool bayonne-runtime ucommon)
target_link_libraries(bayonne-audiotool bayonne-runtime ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})
set_target_properties(bayonne-audiotool PROPERTIES OUTPUT_NAME audiotool)

add_executable(bayonne-control bayonne.cpp)
add_dependencies(bayonne-control bayonne-runtime ucommon)
target_link_libraries(bayonne-control bayonne-runtime ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})
set_target_properties(bayonne-control PROPERTIES OUTPUT_NAME bayonne)

add_executable(bayonne-lint lint.cpp)
add_dependencies(bayonne-lint bayonne-runtime ucommon)
target_link_libraries(bayonne-lint bayonne-runtime ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})
set_target_properties(bayonne-lint PROPERTIES OUTPUT_NAME baylint)

install(TARGETS bayonne-control bayonne-phrasebook bayonne-tonetool bayonne-lint bayonne-audiotool DESTINATION ${INSTALL_BINDIR})


