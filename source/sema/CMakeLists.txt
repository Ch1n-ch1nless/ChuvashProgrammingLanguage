add_library(Sema STATIC
    symbols.cpp
    type_checker.cpp
)

target_include_directories(Sema PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Sema PUBLIC
    Parser
)