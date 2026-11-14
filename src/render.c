#include "render.h"
#include "config.h"
#include "game.h"
#include "dsl_lua.h"
#include "map/chunk.h"
#include <math.h>
#include <limits.h>

static double get_tw() { return TILE_W * game_get_state()->cam_zoom; }
static double get_th() { return TILE_H * game_get_state()->cam_zoom; }

static void grid_to_screen(int mx, int my, int *sx, int *sy) {
    GameState *state = game_get_state();
    *sx = (int)((mx - my) * (get_tw() / 2) + state->cam_offset_x);
    *sy = (int)((mx + my) * (get_th() / 2) + state->cam_offset_y);
}

void render_mouse_to_grid(int screen_x, int screen_y, int *mx, int *my, bool *is_valid) {
    GameState *state = game_get_state();
    double tw = get_tw();
    double th = get_th();

    int dx = screen_x - state->cam_offset_x;
    int dy = screen_y - state->cam_offset_y;

    double fx = ((double)dx / (tw / 2.0) + (double)dy / (th / 2.0)) / 2.0;
    double fy = ((double)dy / (th / 2.0) - (double)dx / (tw / 2.0)) / 2.0;
    int ix = (int)floor(fx + 0.5);
    int iy = (int)floor(fy + 0.5);

    *mx = ix; *my = iy;
    // 无限地图：不再限制 MAP_W/MAP_H，只要坐标在合理范围即有效
    *is_valid = true;
}

static void path_diamond(cairo_t *cr, int sx, int sy, double w, double h) {
    cairo_move_to(cr, sx, sy);
    cairo_line_to(cr, sx + w/2, sy + h/2);
    cairo_line_to(cr, sx, sy + h);
    cairo_line_to(cr, sx - w/2, sy + h/2);
    cairo_close_path(cr);
}

static void draw_ground(cairo_t *cr, int sx, int sy, TileType type, int mx, int my) {
    double tw = get_tw();
    double th = get_th();

    path_diamond(cr, sx, sy, tw, th);
    if (type == TILE_GRASS) cairo_set_source_rgb(cr, 0.30, 0.62, 0.28);
    else if (type == TILE_ROAD) cairo_set_source_rgb(cr, 0.35, 0.35, 0.38);
    else if (type == TILE_HOUSE) cairo_set_source_rgb(cr, 0.25, 0.25, 0.25);
    else cairo_set_source_rgb(cr, 0.28, 0.58, 0.26);

    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.3);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    if (type == TILE_ROAD) {
        cairo_set_source_rgba(cr, 1.0, 1.0, 0.6, 0.8);
        cairo_set_line_width(cr, 1.5 * game_get_state()->cam_zoom);
        cairo_set_dash(cr, (double[]){6.0, 6.0}, 2, 0);

        double cx = sx;
        double cy = sy + th / 2.0;
        double hw = tw / 2.0;
        double hh = th / 2.0;

        bool n = game_is_road(mx, my - 1);
        bool s = game_is_road(mx, my + 1);
        bool w = game_is_road(mx - 1, my);
        bool e = game_is_road(mx + 1, my);

        if (n && s) { cairo_move_to(cr, cx + hw, cy - hh); cairo_line_to(cr, cx - hw, cy + hh); }
        if (w && e) { cairo_move_to(cr, cx - hw, cy - hh); cairo_line_to(cr, cx + hw, cy + hh); }
        if (n && !s && !w && !e) { cairo_move_to(cr, cx, cy); cairo_line_to(cr, cx + hw, cy - hh); }
        if (w && !e && !n && !s) { cairo_move_to(cr, cx, cy); cairo_line_to(cr, cx - hw, cy - hh); }
        if (n && w) { cairo_move_to(cr, cx + hw, cy - hh); cairo_line_to(cr, cx, cy); cairo_line_to(cr, cx - hw, cy - hh); }
        if (n && e) { cairo_move_to(cr, cx + hw, cy - hh); cairo_line_to(cr, cx, cy); cairo_line_to(cr, cx + hw, cy + hh); }
        if (s && w) { cairo_move_to(cr, cx - hw, cy + hh); cairo_line_to(cr, cx, cy); cairo_line_to(cr, cx - hw, cy - hh); }
        if (s && e) { cairo_move_to(cr, cx - hw, cy + hh); cairo_line_to(cr, cx, cy); cairo_line_to(cr, cx + hw, cy + hh); }
        if (n && s && w && e) {
            cairo_move_to(cr, cx - hw, cy - hh); cairo_line_to(cr, cx + hw, cy + hh);
            cairo_move_to(cr, cx + hw, cy - hh); cairo_line_to(cr, cx - hw, cy + hh);
        }

        cairo_stroke(cr);
        cairo_set_dash(cr, NULL, 0, 0);
    }
}

static void draw_building(cairo_t *cr, int sx, int sy, const char *id) {
    if (id && id[0] != '\0') {
        dsl_lua_draw_building(cr, sx, sy, id, game_get_state()->cam_zoom);
    }
}

static void on_draw(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data) {
    (void)area; (void)data;
    GameState *state = game_get_state();
    double tw = get_tw();
    double th = get_th();

    cairo_set_source_rgb(cr, 0.08, 0.10, 0.14);
    cairo_paint(cr);

    double clip_pad = 40.0 * state->cam_zoom + th + 20.0;

    // 屏幕四角反算世界坐标，取 bounding box
    int min_mx = INT_MAX, max_mx = INT_MIN;
    int min_my = INT_MAX, max_my = INT_MIN;
    int corners[4][2] = {{0,0},{width,0},{0,height},{width,height}};
    for (int i = 0; i < 4; i++) {
        int mx, my; bool valid;
        render_mouse_to_grid(corners[i][0], corners[i][1], &mx, &my, &valid);
        if (mx < min_mx) min_mx = mx;
        if (mx > max_mx) max_mx = mx;
        if (my < min_my) min_my = my;
        if (my > max_my) max_my = my;
    }

    // padding 防止边缘闪烁
    min_mx -= 2; max_mx += 2;
    min_my -= 2; max_my += 2;

    for (int my = min_my; my <= max_my; my++) {
        for (int mx = min_mx; mx <= max_mx; mx++) {
            int sx, sy;
            grid_to_screen(mx, my, &sx, &sy);

            if (sx < -tw - clip_pad || sx > width  + tw + clip_pad ||
                sy < -th - clip_pad || sy > height + th + clip_pad) continue;

            int tile = chunk_get_tile(mx, my);
            const char *id = chunk_get_id(mx, my);

            draw_ground(cr, sx, sy, (TileType)tile, mx, my);
            draw_building(cr, sx, sy, id);

            bool is_hover = state->has_hover && state->hover_x == mx && state->hover_y == my;
            bool is_sel = state->has_selection && state->selected_x == mx && state->selected_y == my;

            if (is_hover || is_sel) {
                path_diamond(cr, sx, sy, tw, th);
                if (is_sel) {
                    cairo_set_source_rgba(cr, 1.0, 0.9, 0.1, 0.8);
                    cairo_set_line_width(cr, 2.5 * state->cam_zoom);
                } else {
                    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.6);
                    cairo_set_line_width(cr, 1.5 * state->cam_zoom);
                }
                cairo_stroke(cr);
            }
        }
    }
}

void render_init(GtkDrawingArea *area) {
    gtk_drawing_area_set_draw_func(area, on_draw, NULL, NULL);
}

void render_redraw(GtkWidget *drawing_area) {
    gtk_widget_queue_draw(drawing_area);
}