file(GLOB exosip2_src *.cpp)
file(GLOB exosip2_inc *.h)

add_executable(bayonne-sipw ${exosip2_src} ${exosip2_inc})
add_dependencies(bayonne-sipw bayonne-common bayonne-runtime usecure ucommon eXosip2)
target_link_libraries(bayonne-sipw bayonne-common bayonne-runtime usecure ucommon ${ORTP_LIBS} ${EXOSIP2_LIBS} ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

install(TARGETS bayonne-sipw DESTINATION ${INSTALL_SBINDIR})


