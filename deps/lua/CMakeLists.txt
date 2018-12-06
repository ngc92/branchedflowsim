cmake_minimum_required(VERSION 3.5)

project(lua VERSION 5.3.4 LANGUAGES C)

set(lua_core_src
    src/lapi.c src/lcode.c src/lctype.c src/ldebug.c src/ldo.c src/ldump.c src/lfunc.c src/lgc.c src/llex.c src/lmem.c
    src/lobject.c src/lopcodes.c src/lparser.c src/lstate.c src/lstring.c src/ltable.c src/ltm.c
    src/lundump.c src/lvm.c src/lzio.c)

set(lua_lib_src
    src/lauxlib.c src/lbaselib.c src/lbitlib.c src/lcorolib.c src/ldblib.c src/liolib.c src/lmathlib.c
    src/loslib.c src/lstrlib.c src/ltablib.c src/lutf8lib.c src/loadlib.c src/linit.c)

set(lua_public_headers src/lua.h src/luaconf.h src/lualib.h src/lauxlib.h src/lua.hpp)


add_library(lua STATIC ${lua_core_src} ${lua_lib_src} ${lua_public_headers})
target_include_directories(lua PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(lua::lua ALIAS lua)