#include "character.h"
#include "../json.h"
#include "../dsl.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>

#define MAX_CHARS 32

static CharDef g_chars[MAX_CHARS];
static int     g_count = 0;

static void load_one(const char *path) {
    if (g_count >= MAX_CHARS) return;
    char *text = dsl_read_file(path);
    if (!text) return;

    char err[128] = {0};
    JsonValue *root = json_parse(text, err, sizeof(err));
    free(text);
    if (!root) { fprintf(stderr, "[char] 解析失败 %s: %s\n", path, err); return; }

    CharDef *c = &g_chars[g_count];
    memset(c, 0, sizeof(*c));
    snprintf(c->id, sizeof(c->id), "%s", json_get_string(json_object_get(root, "id"), ""));
    if (c->id[0] == '\0') { json_free(root); return; }
    snprintf(c->name, sizeof(c->name), "%s", json_get_string(json_object_get(root, "name"), c->id));
    snprintf(c->portrait, sizeof(c->portrait), "%s", json_get_string(json_object_get(root, "portrait"), ""));
    c->honesty    = (int)json_get_number(json_object_get(root, "honesty"), 50);
    c->efficiency = (int)json_get_number(json_object_get(root, "efficiency"), 50);
    c->charisma   = (int)json_get_number(json_object_get(root, "charisma"), 50);

    json_free(root);
    g_count++;
}

void character_init(void) {
    g_count = 0;
    DIR *dir = opendir("assets/chars");
    if (!dir) return;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        const char *name = ent->d_name;
        if (name[0] == '.') continue;
        size_t len = strlen(name);
        if (len < 6 || strcmp(name + len - 5, ".json") != 0) continue;
        char full[512];
        snprintf(full, sizeof(full), "assets/chars/%s", name);
        load_one(full);
    }
    closedir(dir);
}

int character_count(void) { return g_count; }
const CharDef* character_get(int idx) {
    if (idx < 0 || idx >= g_count) return NULL;
    return &g_chars[idx];
}
const CharDef* character_find(const char *id) {
    for (int i = 0; i < g_count; i++)
        if (strcmp(g_chars[i].id, id) == 0) return &g_chars[i];
    return NULL;
}