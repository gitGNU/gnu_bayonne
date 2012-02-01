file(GLOB common_src common/*.cpp)
file(GLOB common_inc common/*.h)

add_library(bayonne-common STATIC ${common_src} ${common_inc})
add_dependencies(bayonne-common bayonne-runtime)

if(HAVE_EXOSIP2 AND HAVE_ORTP)
    file(GLOB exosip2_src sipwitch/*.cpp)
    file(GLOB exosip2_inc sipwitch/*.h)

    add_executable(bayonne-sipwitch ${exosip2_src} ${exosip2_inc})
    add_dependencies(bayonne-sipwitch bayonne-common bayonne-runtime usecure ucommon eXosip2)
    target_link_libraries(bayonne-sipwitch bayonne-common bayonne-runtime usecure ucommon ${EXOSIP2_LIBS} ${ORTP_LIBS} ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

    install(TARGETS bayonne-sipwitch DESTINATION ${INSTALL_SBINDIR})
endif()


