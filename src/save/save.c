#include "save.h"
#include "chunk.h"
#include "game.h"
#include "json.h"
#include "../event/event.h"
#include "../story/story.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <windows.h>

// ---------- 路径 ----------
char* save_progress_dir(void) {
    char exe_path[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    if (n == 0) return NULL;

    char *slash = strrchr(exe_path, '\\');
    if (slash) *slash = '\0';

    char dir[MAX_PATH];
    snprintf(dir, sizeof(dir), "%s\\Progress", exe_path);

    g_mkdir_with_parents(dir, 0755);

    return g_strdup(dir);
}

char* save_slot_path(int slot) {
    if (slot < 1 || slot > SAVE_SLOT_COUNT) return NULL;
    char *dir = save_progress_dir();
    if (!dir) return NULL;
    char name[32];
    snprintf(name, sizeof(name), "slot%d.json", slot);
    char *path = g_build_filename(dir, name, NULL);
    g_free(dir);
    return path;
}

bool save_slot_exists(int slot) {
    char *p = save_slot_path(slot);
    if (!p) return false;
    bool ok = g_file_test(p, G_FILE_TEST_EXISTS);
    g_free(p);
    return ok;
}

// ---------- JSON 转义 ----------
static void json_write_escaped(FILE *f, const char *s) {
    fputc('"', f);
    for (const char *p = s; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", f); break;
            case '\\': fputs("\\\\", f); break;
            case '\n': fputs("\\n", f);  break;
            case '\r': fputs("\\r", f);  break;
            case '\t': fputs("\\t", f);  break;
            default:   fputc(*p, f);     break;
        }
    }
    fputc('"', f);
}

// ---------- 保存 ----------
bool save_game(const char *path, const SaveData *data) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "[save] 无法写入 %s\n", path); return false; }

    fprintf(f, "{\n");
    fprintf(f, "  \"version\": %d,\n", SAVE_VERSION);
    fprintf(f, "  \"world_seed\": %u,\n", data->world_seed);
    fprintf(f, "  \"cam\": { \"x\": %.6f, \"y\": %.6f, \"zoom\": %.6f },\n",
            data->cam_offset_x, data->cam_offset_y, data->cam_zoom);
    fprintf(f, "  \"money\": %d,\n", data->money);
    fprintf(f, "  \"population\": %d,\n", data->population);
    fprintf(f, "  \"pop_capacity\": %d,\n", data->pop_capacity);
    fprintf(f, "  \"day\": %d,\n", data->day);

    fprintf(f, "  \"overlay\": [\n");
    for (int i = 0; i < data->overlay_count; i++) {
        fprintf(f, "    { \"x\": %d, \"y\": %d, \"tile\": %d, \"id\": ",
                data->ov_x[i], data->ov_y[i], data->ov_tile[i]);
        json_write_escaped(f, data->ov_id[i] ? data->ov_id[i] : "");
        fprintf(f, " }%s\n", (i + 1 < data->overlay_count) ? "," : "");
    }
    fprintf(f, "  ],\n");

    fprintf(f, "  \"events\": ");
    event_export_state(f);
    fprintf(f, ",\n");

    fprintf(f, "  \"story\": ");
    story_export_state(f);
    fprintf(f, "\n}\n");

    fclose(f);
    return true;
}

// ---------- 读取 ----------
bool load_game(const char *path, SaveData *data) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return false; }
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return false; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);

    char err[128] = {0};
    JsonValue *root = json_parse(buf, err, sizeof(err));
    free(buf);
    if (!root) { fprintf(stderr, "[save] 解析失败: %s\n", err); return false; }

    memset(data, 0, sizeof(*data));
    data->version = (int)json_get_number(json_object_get(root, "version"), 0);
    if (data->version != SAVE_VERSION) {
        fprintf(stderr, "[save] 版本不匹配: %d\n", data->version);
        json_free(root);
        return false;
    }

    data->world_seed = (uint32_t)json_get_number(json_object_get(root, "world_seed"), 1);
    JsonValue *cam = json_object_get(root, "cam");
    if (cam) {
        data->cam_offset_x = json_get_number(json_object_get(cam, "x"), 0);
        data->cam_offset_y = json_get_number(json_object_get(cam, "y"), 0);
        data->cam_zoom     = json_get_number(json_object_get(cam, "zoom"), 1.0);
    }
    data->money        = (int)json_get_number(json_object_get(root, "money"), 1000);
    data->population   = (int)json_get_number(json_object_get(root, "population"), 0);
    data->pop_capacity = (int)json_get_number(json_object_get(root, "pop_capacity"), 0);
    data->day          = (int)json_get_number(json_object_get(root, "day"), 1);

    JsonValue *ov = json_object_get(root, "overlay");
    int count = json_array_size(ov);
    if (count > 0) {
        data->ov_x    = (int*)calloc((size_t)count, sizeof(int));
        data->ov_y    = (int*)calloc((size_t)count, sizeof(int));
        data->ov_tile = (int*)calloc((size_t)count, sizeof(int));
        data->ov_id   = (char**)calloc((size_t)count, sizeof(char*));
        if (!data->ov_x || !data->ov_y || !data->ov_tile || !data->ov_id) {
            json_free(root);
            save_data_free(data);
            return false;
        }
        for (int i = 0; i < count; i++) {
            JsonValue *item = json_array_get(ov, i);
            data->ov_x[i]    = (int)json_get_number(json_object_get(item, "x"), 0);
            data->ov_y[i]    = (int)json_get_number(json_object_get(item, "y"), 0);
            data->ov_tile[i] = (int)json_get_number(json_object_get(item, "tile"), 0);
            const char *id = json_get_string(json_object_get(item, "id"), "");
            size_t len = strlen(id) + 1;
            data->ov_id[i] = (char*)malloc(len);
            if (data->ov_id[i]) memcpy(data->ov_id[i], id, len);
        }
        data->overlay_count = count;
    }

    JsonValue *ev = json_object_get(root, "events");
    if (ev) event_import_from_json(ev);

    JsonValue *st = json_object_get(root, "story");
    if (st) story_import_from_json(st);

    json_free(root);
    return true;
}

// ---------- overlay 收集 ----------
static void collect_visitor(Chunk *c, void *user) {
    SaveData *d = (SaveData*)user;
    for (int ly = 0; ly < CHUNK_SIZE; ly++) {
        for (int lx = 0; lx < CHUNK_SIZE; lx++) {
            if (!c->has_overlay[ly][lx]) continue;
            int wx = c->cx * CHUNK_SIZE + lx;
            int wy = c->cy * CHUNK_SIZE + ly;
            int idx = d->overlay_count++;
            d->ov_x    = (int*)realloc(d->ov_x,    sizeof(int) * (size_t)(idx + 1));
            d->ov_y    = (int*)realloc(d->ov_y,    sizeof(int) * (size_t)(idx + 1));
            d->ov_tile = (int*)realloc(d->ov_tile, sizeof(int) * (size_t)(idx + 1));
            d->ov_id   = (char**)realloc(d->ov_id, sizeof(char*) * (size_t)(idx + 1));
            d->ov_x[idx]    = wx;
            d->ov_y[idx]    = wy;
            d->ov_tile[idx] = c->base_tile[ly][lx];
            d->ov_id[idx]   = (char*)malloc(GRID_ID_LEN);
            if (d->ov_id[idx]) {
                memcpy(d->ov_id[idx], c->grid_id[ly][lx], GRID_ID_LEN);
                d->ov_id[idx][GRID_ID_LEN - 1] = '\0';
            }
        }
    }
}

void save_collect_overlay(SaveData *data) {
    data->overlay_count = 0;
    data->ov_x = data->ov_y = data->ov_tile = NULL;
    data->ov_id = NULL;
    chunk_foreach(collect_visitor, data);
}

void save_apply_overlay(const SaveData *data) {
    for (int i = 0; i < data->overlay_count; i++) {
        chunk_set_overlay(data->ov_x[i], data->ov_y[i],
                          data->ov_tile[i],
                          data->ov_id[i] ? data->ov_id[i] : "");
    }
}

void save_data_free(SaveData *data) {
    if (!data) return;
    for (int i = 0; i < data->overlay_count; i++) free(data->ov_id[i]);
    free(data->ov_x);
    free(data->ov_y);
    free(data->ov_tile);
    free(data->ov_id);
    memset(data, 0, sizeof(*data));
}