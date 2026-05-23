add_library(Token STATIC
    tokenizer.cpp
)

target_include_directories(Token PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

add_library(ToString STATIC
    to_string.cpp
)

target_include_directories(ToString PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ToString PUBLIC
    Token
)