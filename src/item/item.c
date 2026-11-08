#include "item.h"
#include "../json.h"
#include "../dsl.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <cairo/cairo.h>

#define MAX_ITEMS 64

static ItemDef g_items[MAX_ITEMS];
static int     g_count = 0;

// 从 JSON 的 pixels 数组填充 ItemDef 的像素图，返回是否成功
static bool load_pixels(JsonValue *pix, ItemDef *it) {
    int rows = json_array_size(pix);
    if (rows <= 0) return false;
    if (rows > ITEM_MAX_H) rows = ITEM_MAX_H;
    int maxw = 0;
    for (int i = 0; i < rows; i++) {
        const char *line = json_get_string(json_array_get(pix, i), "");
        int len = (int)strlen(line);
        if (len > ITEM_MAX_W) len = ITEM_MAX_W;
        memcpy(it->pixels[i], line, (size_t)len);
        it->pixels[i][len] = '\0';
        if (len > maxw) maxw = len;
    }
    it->height = rows;
    it->width  = maxw;
    return true;
}

// 可选：从 sprite 引用加载像素图
static bool load_sprite(const char *sprite_name, ItemDef *it) {
    char sp[512];
    snprintf(sp, sizeof(sp), "assets/sprites/%s.json", sprite_name);
    char *stext = dsl_read_file(sp);
    if (!stext) return false;

    char err[128] = {0};
    JsonValue *sroot = json_parse(stext, err, sizeof(err));
    free(stext);
    if (!sroot) {
        fprintf(stderr, "[item] sprite 解析失败 %s: %s\n", sp, err);
        return false;
    }
    JsonValue *spix = json_object_get(sroot, "pixels");
    bool ok = load_pixels(spix, it);
    json_free(sroot);
    return ok;
}

static void load_one(const char *path) {
    if (g_count >= MAX_ITEMS) return;

    char *text = dsl_read_file(path);
    if (!text) return;

    char err[128] = {0};
    JsonValue *root = json_parse(text, err, sizeof(err));
    free(text);
    if (!root) {
        fprintf(stderr, "[item] 解析失败 %s: %s\n", path, err);
        return;
    }

    ItemDef *it = &g_items[g_count];
    memset(it, 0, sizeof(*it));

    snprintf(it->id, sizeof(it->id), "%s",
             json_get_string(json_object_get(root, "id"), ""));
    if (it->id[0] == '\0') { json_free(root); return; }

    snprintf(it->name, sizeof(it->name), "%s",
             json_get_string(json_object_get(root, "name"), it->id));

    // item.c 里的 load_one
    snprintf(it->tile, sizeof(it->tile), "%s", json_get_string(json_object_get(root, "tile"), "house"));

    it->cost = (int)json_get_number(json_object_get(root, "cost"), 100);
    if (it->cost < 0) it->cost = 0;

    snprintf(it->sprite, sizeof(it->sprite), "%s",
             json_get_string(json_object_get(root, "sprite"), ""));

    // 像素图：优先用内嵌 pixels；没有则尝试 sprite 引用
    JsonValue *pix = json_object_get(root, "pixels");
    bool has_pixels = load_pixels(pix, it);

    if (!has_pixels && it->sprite[0]) {
        load_sprite(it->sprite, it);
    }

    json_free(root);
    g_count++;
}

void item_init(void) {
    g_count = 0;
    DIR *dir = opendir("assets/items");
    if (!dir) { fprintf(stderr, "[item] 找不到 assets/items\n"); return; }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (name[0] == '.') continue;
        size_t len = strlen(name);
        if (len < 6 || strcmp(name + len - 5, ".json") != 0) continue;
        char full[512];
        snprintf(full, sizeof(full), "assets/items/%s", name);
        load_one(full);
    }
    closedir(dir);
    fprintf(stderr, "[item] 已加载 %d 个物品\n", g_count);
}

int item_count(void) { return g_count; }

const ItemDef* item_get(int idx) {
    if (idx < 0 || idx >= g_count) return NULL;
    return &g_items[idx];
}

const ItemDef* item_find(const char *id) {
    for (int i = 0; i < g_count; i++)
        if (strcmp(g_items[i].id, id) == 0) return &g_items[i];
    return NULL;
}

static void color_for_char(char c, double *r, double *g, double *b) {
    switch (c) {
        case 'G': *r = 0.30; *g = 0.75; *b = 0.30; break;
        case 'B': *r = 0.55; *g = 0.35; *b = 0.20; break;
        case 'Y': *r = 1.00; *g = 0.85; *b = 0.20; break;
        case 'R': *r = 0.85; *g = 0.25; *b = 0.20; break;
        case 'W': *r = 1.00; *g = 1.00; *b = 1.00; break;
        case 'K': *r = 0.10; *g = 0.10; *b = 0.10; break;
        default:  *r = 0.0;  *g = 0.0;  *b = 0.0;  break;
    }
}

void item_render(void *crv, const ItemDef *it, double x, double y, double scale) {
    if (!it) return;
    cairo_t *cr = (cairo_t*)crv;
    for (int row = 0; row < it->height; row++) {
        for (int col = 0; col < it->width; col++) {
            char c = it->pixels[row][col];
            if (c == '.' || c == '\0') continue;
            double r, g, b;
            color_for_char(c, &r, &g, &b);
            cairo_set_source_rgb(cr, r, g, b);
            cairo_rectangle(cr, x + col * scale, y + row * scale, scale, scale);
            cairo_fill(cr);
        }
    }
}