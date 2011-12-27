include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

file(GLOB bayonne_server_src common/*.cpp)
file(GLOB bayonne_server_inc common/*.h)

add_library(bayonne-server STATIC ${bayonne_server_src} ${bayonne_server_inc})
add_dependencies(bayonne-server bayonne-runtime)

if(HAVE_EXOSIP2)
    file(GLOB bayonne_exosip2_src exosip2/*.cpp)
    file(GLOB bayonne_exosip2_inc exosip2/*.h)

    add_executable(bayonne-server-exosip2
        ${bayonne_exosip2_src} ${bayonne_exosip2_inc})
    add_dependencies(bayonne-server-exosip2 bayonne-server bayonne-runtime usecure ucommon eXosip2)
    target_link_libraries(bayonne-server-exosip2 bayonne-server bayonne-runtime usecure ucommon ${EXOSIP2_LIBS} ${USES_UCOMMON_LIBRARIES} ${EXPORT_LIBS})

    install(TARGETS bayonne-server-exosip2 DESTINATION ${INSTALL_SBINDIR})
endif()
