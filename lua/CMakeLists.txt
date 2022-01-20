
# just add library and compile it?

file(GLOB LUA_SRC ./*.c)

add_library(lua
    ${LUA_SRC}
)

if ( CMAKE_COMPILER_IS_GNUCC )
    add_definitions( -DLUA_USE_POSIX )
endif()