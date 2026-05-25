cmake_minimum_required(VERSION 3.16)

add_executable(fx5_client
    include/fx5_client_transport.h
    include/fx5_client_commands.h
    include/fx5_client_app.h
	src/main.c
	src/fx5_client_app.c 
	src/fx5_client_transport.c 
	src/fx5_client_repl.c 
	src/fx5_client_parser.c
        include/fx5_client_repl.h
        include/fx5_client_parser.h
        src/fx5_client_commands.c
        include/fx5_client_config.h
        src/fx5_client_config.c
        include/fx5_client_utility.h
        src/fx5_client_utility.c
)

target_sources(fx5_client PRIVATE
    README.md
    config/fx5_client.ini
)

add_custom_command(TARGET fx5_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/config/fx5_client.ini
        $<TARGET_FILE_DIR:fx5_client>/fx5_client.ini
)

target_compile_definitions(fx5_client
    PRIVATE
        FX5_CLIENT_VERSION_STRING="${PROJECT_VERSION}"
        FX5_CLIENT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FX5_CLIENT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        FX5_CLIENT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_include_directories(fx5_client
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_features(fx5_client PRIVATE c_std_99)

target_link_libraries(fx5_client
    PRIVATE
        fx5_lib
)

   if(WIN32)
       target_link_libraries(fx5_client PRIVATE ws2_32)
   endif()
