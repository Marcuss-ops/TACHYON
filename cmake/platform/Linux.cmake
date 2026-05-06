if(NOT MSVC)
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(TACHYON_ENABLE_ASAN AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()
