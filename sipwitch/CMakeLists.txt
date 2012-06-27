file(GLOB exosip2_src *.cpp)
file(GLOB exosip2_inc *.h)

add_executable(bayonne-sipwitch ${exosip2_src} ${exosip2_inc})
add_dependencies(bayonne-sipwitch bayonne-common bayonne-runtime usecure ucommon eXosip2)
target_link_libraries(bayonne-sipwitch bayonne-common bayonne-runtime usecure ucommon ${EXOSIP2_LIBS} ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

install(TARGETS bayonne-sipwitch DESTINATION ${INSTALL_SBINDIR})


