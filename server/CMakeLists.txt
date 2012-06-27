file(GLOB common_src *.cpp)
file(GLOB common_inc *.h)

add_library(bayonne-common STATIC ${common_src} ${common_inc})
add_dependencies(bayonne-common bayonne-runtime)


