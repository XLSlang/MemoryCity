#ifndef GAME_H
#define GAME_H

#include "config.h"
#include <stdbool.h>

typedef enum {
    TILE_GRASS = 0,
    TILE_ROAD,
    TILE_HOUSE,
    TILE_TREE,
} TileType;

typedef enum {
    TOOL_HOUSE = 0,
    TOOL_ROAD,
    TOOL_BULLDOZE
} BuildTool;

typedef struct {
    TileType grid[MAP_H][MAP_W];
    // 新增：记录每格具体的物品 ID（空字符串表示无）
    char grid_id[MAP_H][MAP_W][32]; 

    int hover_x, hover_y;
    bool has_hover;
    int selected_x, selected_y;
    bool has_selection;

    double cam_offset_x;
    double cam_offset_y;
    double cam_zoom;

    int population;
    int pop_capacity;
    int money;
    int day;

    BuildTool current_tool;

    char     selected_item_id[32];
    bool     use_item_target;
    TileType build_target;
    int      current_cost;
} GameState;

GameState* game_get_state(void);
void game_init(void);
void game_handle_click(int mx, int my, int button);
void game_tick(void);
void game_set_tool(BuildTool tool);

void game_set_item(const char *item_id, const char *tile_name, int cost);
void game_clear_item(void);

TileType game_tile_from_name(const char *name);

bool game_is_road(int x, int y);

#endif // GAME_H