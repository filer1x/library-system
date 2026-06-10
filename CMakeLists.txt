cmake_minimum_required(VERSION 3.16...4.3)

project(LibraryApp LANGUAGES C)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

add_subdirectory(raylib)

add_executable(LibraryApp
    main.c
    library_core.c
    library_core.h
)

target_link_libraries(LibraryApp PRIVATE raylib)
set_target_properties(LibraryApp PROPERTIES
    C_STANDARD 99
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
)

if(MSVC)
    target_compile_options(LibraryApp PRIVATE /utf-8)
elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(LibraryApp PRIVATE -finput-charset=UTF-8 -fexec-charset=UTF-8)
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(LibraryApp PRIVATE -finput-charset=UTF-8)
endif()
