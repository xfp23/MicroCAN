project(MICROCAN C)

file(GLOB_RECURSE MICROCAN_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

add_library(MICROCAN STATIC
    ${MICROCAN_SOURCES}
)


target_include_directories(MICROCAN PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(MICROCAN PRIVATE
    "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>"
)