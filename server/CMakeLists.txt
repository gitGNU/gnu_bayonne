if(HAVE_EXOSIP2 AND HAVE_ORTP)
    file(GLOB exosip2_src exosip2/*.cpp)
    file(GLOB exosip2_inc exosip2/*.h)

    add_executable(bayonne-sipwitch ${exosip2_src} ${exosip2_inc})
    add_dependencies(bayonne-sipwitch bayonne-server bayonne-runtime usecure ucommon eXosip2)
    target_link_libraries(bayonne-sipwitch bayonne-server bayonne-runtime usecure ucommon ${EXOSIP2_LIBS} ${ORTP_LIBS} ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

    install(TARGETS bayonne-sipwitch DESTINATION ${INSTALL_SBINDIR})
endif()


