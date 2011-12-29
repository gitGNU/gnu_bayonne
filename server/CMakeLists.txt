if(HAVE_EXOSIP2)
    file(GLOB exosip2_src exosip2/*.cpp)
    file(GLOB exosip2_inc exosip2/*.h)

    add_executable(bayonne-server-exosip2 ${exosip2_src} ${exosip2_inc})
    add_dependencies(bayonne-server-exosip2 bayonne-server bayonne-runtime usecure ucommon eXosip2)
    target_link_libraries(bayonne-server-exosip2 bayonne-server bayonne-runtime usecure ucommon ${EXOSIP2_LIBS} ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

    install(TARGETS bayonne-server-exosip2 DESTINATION ${INSTALL_SBINDIR})
endif()

if(HAVE_VPBAPI)
    file(GLOB vpbapi_src vpbapi/*.cpp)
    file(GLOB vpbapi_inc vpbapi/*.h)

    add_executable(bayonne-server-vpbapi ${vpbapi_src} ${vpbapi_inc})
    add_dependencies(bayonne-server-vpbapi bayonne-server bayonne-runtime ucommon)
    target_link_libraries(bayonne-server-vpbapi bayonne-server bayonne-runtime ucommon vpb ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

    install(TARGETS bayonne-server-vpbapi DESTINATION ${INSTALL_SBINDIR})
endif()
