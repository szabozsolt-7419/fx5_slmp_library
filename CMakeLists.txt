cmake_minimum_required(VERSION 3.16)

project(fx5_simple_c_library VERSION 0.1.0 LANGUAGES C)

enable_testing()

add_library(fx5_lib STATIC
  include/fx5/fx5.h
  include/fx5/internal/fx5_private.h
  include/fx5/internal/fx5_utility.h  
  src/fx5_core.c
  src/fx5_request.c
  src/fx5_response.c
  include/fx5/fx5_ringbuf.h
  src/fx5_ringbuf.c
  include/fx5/fx5_layout.h
)

target_sources(fx5_lib PRIVATE
  README.md
  Doxyfile
  LICENSE
)


target_compile_definitions(fx5_lib
    PUBLIC
        FX5_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        FX5_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        FX5_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_include_directories(fx5_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_custom_target(docs
  COMMAND doxygen ${CMAKE_SOURCE_DIR}/Doxyfile
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# --- Unity tests -------------------------------------------------------------

add_executable(fx5_tests
  tests/test_main.c
  tests/test_fx5_pdu_vectors.c
  tests/test_fx5_vectors.h
  tests/unity/unity.c
  tests/unity/unity.h
  tests/unity/unity_internals.h
  tests/test_fx5_parser_vectors.c
  tests/test_fx5_edge_cases.c
  tests/test_fx5_client_parser.c
  tests/test_support.h
  tools/fx5_client/src/fx5_client_parser.c
  tools/fx5_client/src/fx5_client_utility.c

)

target_include_directories(fx5_tests
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tools/fx5_client/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/unity
)

target_link_libraries(fx5_tests
  PRIVATE
    fx5_lib
)


add_test(NAME fx5_tests COMMAND fx5_tests)


option(FX5_BUILD_TOOLS "Build command-line tools" ON)

if (FX5_BUILD_TOOLS)
  add_subdirectory(tools/fx5_client)

 endif()

