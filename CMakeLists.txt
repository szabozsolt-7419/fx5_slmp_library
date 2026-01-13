cmake_minimum_required(VERSION 3.16)

project(fx5_simple_c_library LANGUAGES CXX C)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Test)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Test)


add_library(fx5_lib STATIC
  include/fx5.h
  src/fx5_private.h
  #melsec_pdu_request_parser.c
  #src/melsec_pdu.h
  #melsec_pdu_private.h
  #melsec_pdu_request.h
  #melsec_pdu_response.h
  #melsec_pdu_request_builder.c
  #melsec_pdu_utility.c
  src/buffer.c
  src/fx5.c
)

add_custom_target(docs
    COMMAND doxygen ${CMAKE_SOURCE_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)


target_include_directories(fx5_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(fx5_c_library
  README.md
  src/main.cpp
)

target_link_libraries(fx5_c_library PRIVATE Qt${QT_VERSION_MAJOR}::Core fx5_lib)

# --- tests ---
enable_testing()

add_executable(fx5_lib_tests
  src/test_fx5_lib.cpp
)
target_link_libraries(fx5_lib_tests PRIVATE Qt${QT_VERSION_MAJOR}::Test fx5_lib)

add_test(NAME fx5_lib_tests COMMAND fx5_lib_tests)


