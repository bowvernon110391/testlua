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

static bool testLoaded(lua_State *l, const char* name);

// our custom loader. for now, just return error
static int luaf_customLoad(lua_State *l) {
    if (!lua_gettop(l)) {
        printf("CL: no argument for loader.\n");
    }

    if (!lua_isstring(l, -1)) {
        printf("CL: non string argument provided for loader.\n");
    }

    const char* name = lua_tostring(l, -1);
    lua_pop(l, 1);

    // check if loaded
    if (testLoaded(l, name)) {
        printf("CL: module[ %s ] already loaded\n", name);
        // then just load em
        lua_getfield(l, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_getfield(l, -1, name);
        lua_rotate(l, -2, -1);
        lua_pop(l, 1);
        return 1;
    }

    printf("CL: load -> %s\n", name);

    char buffer[256];
    sprintf(buffer, "%s.lua", name);

    if (luaL_loadfile(l, buffer)) {
        printf("CL: error loading [%s] -> %s\n", buffer, lua_tostring(l, -1));
        lua_pop(l, 1);
        return 0;
    }

    // it's loaded?
    // then the function (loader) will be at the top. check em?
    const void* loader = lua_topointer(l, -1);
    printf("CL: loader at (0x%p), ready to be called.\n", loader);

    if (lua_pcall(l, 0, LUA_MULTRET, 0)) {
        printf("CL: loader failed -> %s\n", lua_tostring(l, -1));
        lua_pop(l, 1);
    }

    // do what now?
    // compute stack?
    int stackSize = lua_gettop(l);
    printf("CL: stack after loader -> %d\n", stackSize);

    if (stackSize) {
        const char* retType = lua_typename(l, lua_type(l, -1));
        const void* ptr = lua_topointer(l, -1);
        printf("CL: loader return %s at (0x%p)\n", retType, ptr);

        // store it?
        lua_getfield(l, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        // lua_getfield(l, -1, name);
        lua_pushstring(l, name);

        // check stack size
        printf("CL: ready to store, since we have %d at stack\n", lua_gettop(l));
        printf("-- stack contents (PRE-ROTATION) [ \n");
        for (int i=0; i<lua_gettop(l); i++) {
            int idx = -1 - i;
            printf("\t%s\n", lua_typename(l, lua_type(l, idx)));
        }
        printf(" ]\n");

        // rotate it
        lua_rotate(l, -3, -1);

        printf("-- stack contents (POST-ROTATION) [ \n");
        for (int i=0; i<lua_gettop(l); i++) {
            int idx = -1 - i;
            printf("\t%s\n", lua_typename(l, lua_type(l, idx)));
        }
        printf(" ]\n");

        // just call settable now
        lua_settable(l, -3);

        printf("-- stack size after settable -> %d\n", lua_gettop(l));

        // after this, the key and value are popped, leaving the table at top of the stack
        // reload the table from registry instead!
        lua_pop(l, 1);  // clear stack
        // reload
        lua_getfield(l, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_getfield(l, -1, name);
        lua_rotate(l, -2, -1);
        lua_pop(l, 1);

        return 1;
    } else {
        printf("CL: Loader doesn't return anything\n");
        printf("-- stack contents (PRE-ASSIGNMENT) [ \n");
        for (int i=0; i<lua_gettop(l); i++) {
            int idx = -1 - i;
            printf("\t%s\n", lua_typename(l, lua_type(l, idx)));
        }
        printf(" ]\n");

        // push boolean and set
        lua_getfield(l, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
        lua_pushstring(l, name);
        lua_pushboolean(l, true);

        lua_settable(l, -3);
        lua_pop(l, 1);

        lua_pushboolean(l, true);
        return 1;   // push 1 value to stack?
    }

    lua_settop(l, 0);
    return 0;
}

static luaL_Reg funcs[] = {
    { "require", luaf_customLoad },
    { NULL, NULL }
};

#ifdef __cplusplus
}
#endif

// add our loader (basically replace the lua require loader)
static void reg_loader(lua_State *l) {
    lua_pushglobaltable(l);
    lua_pushvalue(l, -2);
    luaL_setfuncs(l, funcs, 1);
    lua_pop(l, 1);
}

static bool testLoaded(lua_State *l, const char* name) {
    printf("before getfield: %d\n", lua_gettop(l));
    // test read?
    lua_getfield(l, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    printf("after first: %d\n", lua_gettop(l));
    // push string?
    lua_getfield(l, -1, name);
    printf("after second: %d\n", lua_gettop(l));

    // let's read it?
    const void* ptr = lua_topointer(l, -1);
    int type = lua_type(l, -1);
    printf("Top stack type: %s at 0x%p\n", lua_typename(l, type), ptr);
    // grab ptr

    // lua_settop(l, 0);
    lua_pop(l, 2);

    return type != LUA_TNIL;
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
    // testLoaded(l, "module/common");

    printf("loading [%s]\n", argv[1]);
    if (luaL_loadfile(l, argv[1])) {
        printf("Error loading [%s]: %s\n", argv[1], lua_tostring(l, -1));
        lua_pop(l, 1);
    } else {
        // name it
        printf("Loaded %s to stack id %d\n", argv[1], lua_gettop(l));
        lua_setglobal(l, argv[1]);

        // run it several times
        for (int i=0; i<3; i++) {
            lua_getglobal(l, argv[1]);
            if (lua_pcall(l, 0, LUA_MULTRET, 0)) {
                printf("Error running [%s]: %s\n", argv[1], lua_tostring(l, -1));
                lua_pop(l, 1);
            }
        }
    }

    // testLoaded(l, "module/common");
   
    lua_close(l);

    return 0;
}