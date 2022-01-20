#include <stdio.h>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

static int luaf_getAreaSize(lua_State* l) {
    double width = luaL_checknumber(l, 1);
    double height = luaL_checknumber(l, 2);

    double area = width * height;

    printf("CPP: called getAreaSize() with param (%f, %f)\n", (float)width, (float)height);

    lua_pushnumber(l, area);
    return 1;
}

// our custom loader. for now, just return error
static int luaf_customLoad(lua_State *l) {
    printf("CUSTOM_LOADER loading...\n");
    const char* name = luaL_checkstring(l, 1);
    printf("CUSTOM_LOADER: loading module [%s]\n", name);

    // push back the name?
    char filename[256];
    sprintf(filename, "%s.lua", name);
    int state = luaL_loadfile(l, filename);

    if (state) {
        printf("error loading module [%s]: %s\n", name, lua_tostring(l, -1));
        lua_pop(l, 1);
    } else {
        // call it?
        state = lua_pcall(l, 0, LUA_MULTRET, 0);
        if (state) {
            printf("Error running module [%s]: %s\n", name, lua_tostring(l, -1));
            lua_pop(l, 1);
        }
    }

    return 1;
}

static luaL_Reg funcs[] = {
    { "require", luaf_customLoad },
    { NULL, NULL }
};

#ifdef __cplusplus
}
#endif

// add our loader
static void reg_loader(lua_State *l) {
    lua_pushglobaltable(l);
    lua_pushvalue(l, -2);
    luaL_setfuncs(l, funcs, 1);
    lua_pop(l, 1);
}

int main(int argc, char** argv) {

    if (argc < 2) {
        printf("USAGE: test [luafile]\n");
        return 0;
    }

    // do the real shit
    lua_State *l = luaL_newstate();

    luaL_openlibs(l);

    // register our custom preload fn?
    reg_loader(l);

    printf("loading [%s]\n", argv[1]);
    if (luaL_loadfile(l, argv[1])) {
        printf("Error loading [%s]: %s\n", argv[1], lua_tostring(l, -1));
        lua_pop(l, 1);
    } else {
        // push variable
        lua_pushnumber(l, 23);
        lua_setglobal(l, "CLICK_MULTIPLIER");

        // push function
        lua_pushcfunction(l, luaf_getAreaSize);
        lua_setglobal(l, "getAreaSize");

        // run it
        if (lua_pcall(l, 0, LUA_MULTRET, 0)) {
            printf("Error running [%s]: %s\n", argv[1], lua_tostring(l, -1));
            lua_pop(l, 1);
        }
    }

    // read lua variable
    lua_getglobal(l, "click_count");
    int click_count = (int) lua_tonumber(l, -1);
    lua_pop(l, 1);

    printf("the [click_count] value is %d\n", click_count);

    lua_close(l);

    return 0;
}