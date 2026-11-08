#include "event.h"
#include "game.h"
#include "../json.h"
#include "../dsl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define MAX_EVENTS 64

static EventDef  g_events[MAX_EVENTS];
static int       g_count = 0;
static bool      g_active = false;
static int       g_current = -1;

static void load_one(const char *path) {
    if (g_count >= MAX_EVENTS) return;

    char *text = dsl_read_file(path);   // dsl.c 的读文件工具，通用
    if (!text) return;

    char err[128] = {0};
    JsonValue *root = json_parse(text, err, sizeof(err));
    free(text);
    if (!root) {
        fprintf(stderr, "[event] 解析 %s 失败: %s\n", path, err);
        return;
    }
    if (root->type != JSON_OBJECT) {
        fprintf(stderr, "[event] %s 根不是对象\n", path);
        json_free(root);
        return;
    }

    EventDef *ev = &g_events[g_count];
    memset(ev, 0, sizeof(*ev));

    snprintf(ev->id, sizeof(ev->id), "%s", json_get_string(json_object_get(root, "id"), ""));
    if (ev->id[0] == '\0') { json_free(root); return; }
    snprintf(ev->title, sizeof(ev->title), "%s", json_get_string(json_object_get(root, "title"), ev->id));
    snprintf(ev->desc, sizeof(ev->desc), "%s", json_get_string(json_object_get(root, "desc"), ""));
    snprintf(ev->portrait, sizeof(ev->portrait), "%s", json_get_string(json_object_get(root, "portrait"), ""));

    JsonValue *cond = json_object_get(root, "condition");
    if (cond && cond->type == JSON_OBJECT) {
        ev->cond_min_day = (int)json_get_number(json_object_get(cond, "min_day"), 0);
        ev->cond_min_pop = (int)json_get_number(json_object_get(cond, "min_pop"), 0);
    }
    ev->cond_prob = (int)json_get_number(json_object_get(root, "probability"), 0);

    JsonValue *choices = json_object_get(root, "choices");
    int n = json_array_size(choices);
    for (int i = 0; i < n && i < EVENT_MAX_CHOICES; i++) {
        JsonValue *c = json_array_get(choices, i);
        if (!c || c->type != JSON_OBJECT) continue;
        int idx = ev->choice_count++;
        snprintf(ev->choice_text[idx], sizeof(ev->choice_text[idx]),
                 "%s", json_get_string(json_object_get(c, "text"), "..."));
        ev->choice_money[idx] = (int)json_get_number(json_object_get(c, "money"), 0);
        ev->choice_pop[idx]   = (int)json_get_number(json_object_get(c, "pop"), 0);
        snprintf(ev->choice_result[idx], sizeof(ev->choice_result[idx]),
                 "%s", json_get_string(json_object_get(c, "result"), ""));
    }

    json_free(root);
    g_count++;
}

void event_init(void) {
    g_count = 0;
    g_active = false;
    g_current = -1;

    const char *dir_path = "assets/events";
    DIR *dir = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "[event] 找不到目录 %s（跳过事件加载）\n", dir_path);
        return;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (name[0] == '.') continue;
        size_t len = strlen(name);
        if (len < 6 || strcmp(name + len - 5, ".json") != 0) continue;
        char full[512];
        snprintf(full, sizeof(full), "%s/%s", dir_path, name);
        load_one(full);
    }
    closedir(dir);
    fprintf(stderr, "[event] 已加载 %d 个事件\n", g_count);
}

int event_count(void) { return g_count; }
bool event_is_active(void) { return g_active; }
const EventDef* event_current(void) {
    if (!g_active || g_current < 0) return NULL;
    return &g_events[g_current];
}

bool event_try_trigger(void) {
    if (g_active || g_count == 0) return false;
    GameState *st = game_get_state();

    int candidates[MAX_EVENTS];
    int nc = 0;
    for (int i = 0; i < g_count; i++) {
        EventDef *e = &g_events[i];
        if (e->fired) continue;                       // 已触发过，跳过
        if (e->cond_min_day && st->day < e->cond_min_day) continue;
        if (e->cond_min_pop && st->population < e->cond_min_pop) continue;
        if (e->cond_prob <= 0) continue;
        if ((rand() % 100) < e->cond_prob) candidates[nc++] = i;
    }
    if (nc == 0) return false;
    g_current = candidates[rand() % nc];
    g_active = true;
    g_events[g_current].fired = true;                 // 标记已触发
    return true;
}

bool event_force_trigger(const char *id) {
    if (g_active) return false;
    for (int i = 0; i < g_count; i++) {
        if (strcmp(g_events[i].id, id) == 0) {
            g_current = i;
            g_active = true;
            g_events[i].fired = true;                 // 标记已触发
            return true;
        }
    }
    return false;
}

void event_apply_choice(int idx) {
    if (!g_active) return;
    EventDef *e = &g_events[g_current];
    if (idx < 0 || idx >= e->choice_count) { event_close(); return; }

    GameState *st = game_get_state();
    st->money += e->choice_money[idx];
    st->population += e->choice_pop[idx];
    if (st->population < 0) st->population = 0;
    if (st->population > st->pop_capacity) st->population = st->pop_capacity;
    if (st->money < 0) st->money = 0;
    event_close();
}

void event_close(void) {
    g_active = false;
    g_current = -1;
}