if(HAVE_EXOSIP2)
    file(GLOB exosip2_src exosip2/*.cpp)
    file(GLOB exosip2_inc exosip2/*.h)

    add_executable(bayonne-server-exosip2 ${exosip2_src} ${exosip2_inc})
    add_dependencies(bayonne-server-exosip2 bayonne-server bayonne-runtime usecure ucommon eXosip2)
    target_link_libraries(bayonne-server-exosip2 bayonne-server bayonne-runtime usecure ucommon ${EXOSIP2_LIBS} ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

    install(TARGETS bayonne-server-exosip2 DESTINATION ${INSTALL_SBINDIR})
endif()
