#include "game.h"
#include <stdio.h>
#include <string.h>

static GameState g_state;

GameState* game_get_state(void) { return &g_state; }

TileType game_tile_from_name(const char *name) {
    if (!name) return TILE_GRASS;
    if (strcmp(name, "house") == 0) return TILE_HOUSE;
    if (strcmp(name, "road")  == 0) return TILE_ROAD;
    if (strcmp(name, "tree")  == 0) return TILE_TREE;
    if (strcmp(name, "grass") == 0) return TILE_GRASS;
    return TILE_GRASS;
}

void game_init(void) {
    for (int y = 0; y < MAP_H; y++) {
        for (int x = 0; x < MAP_W; x++) {
            g_state.grid[y][x] = TILE_GRASS;
            g_state.grid_id[y][x][0] = '\0';
        }
    }

    g_state.has_hover = false;
    g_state.has_selection = false;
    g_state.cam_offset_x = 0;
    g_state.cam_offset_y = 0;
    g_state.cam_zoom = 1.0;
    g_state.population = 0;
    g_state.pop_capacity = 0;
    g_state.money = 1000;
    g_state.day = 1;
    g_state.current_tool = TOOL_HOUSE;

    g_state.selected_item_id[0] = '\0';
    g_state.use_item_target = false;
    g_state.build_target = TILE_HOUSE;
    g_state.current_cost = 100;
}

void game_set_tool(BuildTool tool) {
    g_state.current_tool = tool;
    g_state.selected_item_id[0] = '\0';
    g_state.use_item_target = false;
    switch (tool) {
        case TOOL_HOUSE:    g_state.current_cost = 100; break;
        case TOOL_ROAD:     g_state.current_cost = 10;  break;
        case TOOL_BULLDOZE: g_state.current_cost = 0;   break;
    }
}

void game_set_item(const char *item_id, const char *tile_name, int cost) {
    snprintf(g_state.selected_item_id, sizeof(g_state.selected_item_id), "%s",
             item_id ? item_id : "");

    TileType t = game_tile_from_name(tile_name);
    g_state.use_item_target = true;
    g_state.build_target = t;
    g_state.current_cost = cost;

    if (t == TILE_HOUSE)      g_state.current_tool = TOOL_HOUSE;
    else if (t == TILE_ROAD)  g_state.current_tool = TOOL_ROAD;
    else                      g_state.current_tool = TOOL_BULLDOZE;
}

void game_clear_item(void) {
    g_state.selected_item_id[0] = '\0';
    g_state.use_item_target = false;
}

static TileType tool_to_tile(void) {
    switch (g_state.current_tool) {
        case TOOL_HOUSE:    return TILE_HOUSE;
        case TOOL_ROAD:     return TILE_ROAD;
        case TOOL_BULLDOZE: return TILE_GRASS;
    }
    return TILE_GRASS;
}

void game_handle_click(int mx, int my, int button) {
    if (mx < 0 || mx >= MAP_W || my < 0 || my >= MAP_H) return;

    // 右键拆除
    if (button == 3) {
        if (g_state.grid[my][mx] != TILE_GRASS) {
            g_state.grid[my][mx] = TILE_GRASS;
            g_state.grid_id[my][mx][0] = '\0'; // 清空 ID
            g_state.money += 10;
        }
        return;
    }

    if (button != 1) return;

    TileType current = g_state.grid[my][mx];
    TileType target = g_state.use_item_target ? g_state.build_target : tool_to_tile();

    if (target == TILE_GRASS) {
        if (current != TILE_GRASS) {
            g_state.grid[my][mx] = TILE_GRASS;
            g_state.grid_id[my][mx][0] = '\0'; // 清空 ID
        }
        return;
    }

    // 建造
    if (current != TILE_GRASS) return;
    int cost = g_state.current_cost;
    if (g_state.money < cost) return;

    g_state.grid[my][mx] = target;
    g_state.money -= cost;
    
    // 存入具体的物品 ID
    if (g_state.use_item_target && g_state.selected_item_id[0] != '\0') {
        snprintf(g_state.grid_id[my][mx], 32, "%s", g_state.selected_item_id);
    } else {
        if (target == TILE_HOUSE) snprintf(g_state.grid_id[my][mx], 32, "house");
        else if (target == TILE_ROAD) snprintf(g_state.grid_id[my][mx], 32, "road");
    }
}

void game_tick(void) {
    int houses = 0;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++)
            if (g_state.grid[y][x] == TILE_HOUSE) houses++;

    g_state.pop_capacity = houses * 10;

    if (g_state.population < g_state.pop_capacity) {
        g_state.population += (g_state.pop_capacity - g_state.population) / 5 + 1;
        if (g_state.population > g_state.pop_capacity)
            g_state.population = g_state.pop_capacity;
    } else if (g_state.population > g_state.pop_capacity) {
        g_state.population -= (g_state.population - g_state.pop_capacity) / 2 + 1;
        if (g_state.population < g_state.pop_capacity)
            g_state.population = g_state.pop_capacity;
    }

    g_state.money += g_state.population / 2;
    g_state.day++;
}

bool game_is_road(int x, int y) {
    if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return false;
    return g_state.grid[y][x] == TILE_ROAD;
}