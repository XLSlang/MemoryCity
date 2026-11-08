#include "dsl_lua.h"    // 包含我们自定义的头文件
#include <lua.h>        // 包含官方 Lua 头文件
#include <lauxlib.h>
#include <lualib.h>
#include <dirent.h>
#include <stdio.h>

static lua_State *L = NULL;
static int has_errored = 0; // 用于标记 Lua 错误状态

// ---------- 封装 cairo 接口给 Lua ----------
static int l_move_to(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_move_to(cr, luaL_checknumber(l, 2), luaL_checknumber(l, 3));
    return 0;
}

static int l_line_to(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_line_to(cr, luaL_checknumber(l, 2), luaL_checknumber(l, 3));
    return 0;
}

static int l_set_rgb(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_set_source_rgb(cr, luaL_checknumber(l, 2), luaL_checknumber(l, 3), luaL_checknumber(l, 4));
    return 0;
}

static int l_set_rgba(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_set_source_rgba(cr, luaL_checknumber(l, 2), luaL_checknumber(l, 3), luaL_checknumber(l, 4), luaL_checknumber(l, 5));
    return 0;
}

static int l_fill(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_fill(cr);
    return 0;
}

static int l_fill_preserve(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_fill_preserve(cr);
    return 0;
}

static int l_stroke(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_stroke(cr);
    return 0;
}

static int l_set_line_width(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_set_line_width(cr, luaL_checknumber(l, 2));
    return 0;
}

static int l_close_path(lua_State *l) {
    cairo_t *cr = (cairo_t*)lua_touserdata(l, 1);
    cairo_close_path(cr);
    return 0;
}

// ---------- 生命周期 ----------
void dsl_lua_init(void) {
    L = luaL_newstate();
    if (!L) return;
    luaL_openlibs(L);

    // 注册 cairo 函数 (和之前一样)
    lua_register(L, "move_to", l_move_to);
    lua_register(L, "line_to", l_line_to);
    lua_register(L, "set_rgb", l_set_rgb);
    lua_register(L, "set_rgba", l_set_rgba);
    lua_register(L, "fill", l_fill);
    lua_register(L, "fill_preserve", l_fill_preserve);
    lua_register(L, "stroke", l_stroke);
    lua_register(L, "set_line_width", l_set_line_width);
    lua_register(L, "close_path", l_close_path);

    // 1. 先加载调度中心
    if (luaL_dofile(L, "assets/scripts/buildings.lua") != LUA_OK) {
        fprintf(stderr, "[lua] 加载 buildings.lua 失败: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1);
    }

    // 2. 扫描并加载 models 目录下的所有模型脚本
    DIR *dir = opendir("assets/scripts/models");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            const char *name = ent->d_name;
            if (name[0] == '.') continue; // 忽略 . 和 ..
            size_t len = strlen(name);
            if (len > 4 && strcmp(name + len - 4, ".lua") == 0) {
                char full_path[512];
                snprintf(full_path, sizeof(full_path), "assets/scripts/models/%s", name);
                if (luaL_dofile(L, full_path) != LUA_OK) {
                    fprintf(stderr, "[lua] 加载模型 %s 失败: %s\n", name, lua_tostring(L, -1));
                    lua_pop(L, 1);
                } else {
                    fprintf(stderr, "[lua] 成功加载模型: %s\n", name);
                }
            }
        }
        closedir(dir);
    }
    
    fprintf(stderr, "[lua] Lua 渲染引擎初始化完毕\n");
}

void dsl_lua_draw_building(cairo_t *cr, int sx, int sy, const char *tile_id, double zoom) {
    if (!L) return;

    lua_getglobal(L, "draw_building");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }

    lua_pushlightuserdata(L, cr);
    lua_pushinteger(L, sx);
    lua_pushinteger(L, sy);
    lua_pushnumber(L, zoom);
    lua_pushstring(L, tile_id);

    if (lua_pcall(L, 5, 0, 0) != LUA_OK) {
        // 如果报错，且还没打印过，才打印一次
        if (!has_errored) {
            fprintf(stderr, "[lua] 渲染运行错误 (%s): %s\n", tile_id, lua_tostring(L, -1));
            has_errored = 1; 
        }
        lua_pop(L, 1); // 清除错误信息
    }
}

void dsl_lua_close(void) {
    if (L) {
        lua_close(L); // 调用官方 lua_close
        L = NULL;
    }
}