add_library(Driver STATIC
    compiler_driver.cpp
    linker_driver.cpp
)

target_include_directories(Driver PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Driver PUBLIC

    Token
    ToString
    Parser
    Sema
    Codegen
    ObjectEmitter
)