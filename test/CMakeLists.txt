include_directories(${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..inc)

link_directories(${CMAKE_CURRENT_BINARY_DIR}/../runtime)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/mergescript.scr
    ${CMAKE_CURRENT_BINARY_DIR}/mergescript.scr COPYONLY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/testscript.scr
    ${CMAKE_CURRENT_BINARY_DIR}/testscript.scr COPYONLY)

add_executable(test-bayonneScript script.cpp)
target_link_libraries(test-bayonneScript bayonne-runtime)
add_test(NAME bayonneScript COMMAND test-bayonneScript)

