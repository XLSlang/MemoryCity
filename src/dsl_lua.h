#ifndef DSL_LUA_H
#define DSL_LUA_H

#include <cairo/cairo.h>

void dsl_lua_init(void);
void dsl_lua_draw_building(cairo_t *cr, int sx, int sy, const char *tile_id, double zoom);
void dsl_lua_close(void);

#endif