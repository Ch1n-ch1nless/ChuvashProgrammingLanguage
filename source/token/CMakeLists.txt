add_library(Token STATIC tokenizer.cpp)
add_library(ToString STATIC to_string.cpp)
target_link_libraries(Token ToString)