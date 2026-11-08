#include "story.h"
#include "../json.h"
#include "../dsl.h"
#include "../event/event.h"
#include "../game.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define MAX_STEPS 32

typedef struct {
    char event_id[32];
    int  delay;
} StoryStep;

typedef struct {
    char chapter[32];
    int  cond_min_day;
    int  step_count;
    StoryStep steps[MAX_STEPS];
} StoryDef;

static StoryDef g_story;
static bool     g_loaded = false;
static int      g_step_index = 0;
static int      g_tick_counter = 0;
static bool     g_running = false;

static void load_one(const char *path) {
    char *text = dsl_read_file(path);
    if (!text) return;

    char err[128] = {0};
    JsonValue *root = json_parse(text, err, sizeof(err));
    free(text);
    if (!root) { fprintf(stderr, "[story] 解析失败 %s: %s\n", path, err); return; }

    memset(&g_story, 0, sizeof(g_story));
    snprintf(g_story.chapter, sizeof(g_story.chapter),
             "%s", json_get_string(json_object_get(root, "chapter"), "1"));
    g_story.cond_min_day = (int)json_get_number(json_object_get(root, "min_day"), 1);

    JsonValue *steps = json_object_get(root, "steps");
    int n = json_array_size(steps);
    for (int i = 0; i < n && i < MAX_STEPS; i++) {
        JsonValue *s = json_array_get(steps, i);
        if (!s || s->type != JSON_OBJECT) continue;
        StoryStep *st = &g_story.steps[g_story.step_count];
        snprintf(st->event_id, sizeof(st->event_id),
                 "%s", json_get_string(json_object_get(s, "event_id"), ""));
        st->delay = (int)json_get_number(json_object_get(s, "delay"), 0);
        if (st->event_id[0]) g_story.step_count++;
    }
    json_free(root);
    g_loaded = true;
    fprintf(stderr, "[story] 已加载 chapter=%s steps=%d\n", g_story.chapter, g_story.step_count);
}

void story_init(void) {
    memset(&g_story, 0, sizeof(g_story));
    g_loaded = false;
    g_step_index = 0;
    g_tick_counter = 0;
    g_running = false;

    DIR *dir = opendir("assets/story");
    if (!dir) { fprintf(stderr, "[story] 找不到 assets/story\n"); return; }
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (name[0] == '.') continue;
        size_t len = strlen(name);
        if (len < 6 || strcmp(name + len - 5, ".json") != 0) continue;
        char full[512];
        snprintf(full, sizeof(full), "assets/story/%s", name);
        load_one(full);
        break;
    }
    closedir(dir);
}

void story_tick(void) {
    if (!g_loaded || g_story.step_count == 0) return;
    GameState *st = game_get_state();
    if (st->day < g_story.cond_min_day) return;
    if (event_is_active()) return;

    if (!g_running) {
        if (g_step_index >= g_story.step_count) return;
        StoryStep *s = &g_story.steps[g_step_index];
        if (g_tick_counter >= s->delay) {
            if (event_force_trigger(s->event_id)) g_running = true;
            else { g_step_index++; g_tick_counter = 0; }
        } else {
            g_tick_counter++;
        }
    } else {
        if (!event_is_active()) {
            g_running = false;
            g_step_index++;
            g_tick_counter = 0;
        }
    }
}

bool story_active(void) { return g_running && event_is_active(); }
const char* story_current_text(void) {
    if (!g_loaded || g_step_index >= g_story.step_count) return "";
    return g_story.steps[g_step_index].event_id;
}