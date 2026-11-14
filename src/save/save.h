#ifndef SAVE_H
#define SAVE_H

#include <stdbool.h>
#include <stdint.h>

#define SAVE_VERSION 1
#define SAVE_SLOT_COUNT 3

typedef struct {
    int version;
    uint32_t world_seed;

    double cam_offset_x, cam_offset_y;
    double cam_zoom;

    int money;
    int population;
    int pop_capacity;
    int day;

    int overlay_count;
    int *ov_x;
    int *ov_y;
    int *ov_tile;
    char **ov_id;
} SaveData;

// 存档目录：可执行文件同级 Progress/
char* save_progress_dir(void);

// 槽位路径：Progress/slotN.json
char* save_slot_path(int slot);

// 槽位是否存在
bool save_slot_exists(int slot);

bool save_game(const char *path, const SaveData *data);
bool load_game(const char *path, SaveData *data);

void save_collect_overlay(SaveData *data);
void save_apply_overlay(const SaveData *data);
void save_data_free(SaveData *data);

#endif // SAVE_H