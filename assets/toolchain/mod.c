#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>

// 去掉首尾空白
void trim(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace(str[len - 1])) str[--len] = '\0';
    int start = 0;
    while (str[start] && isspace(str[start])) start++;
    if (start > 0) memmove(str, str + start, len - start + 1);
}

// 确保目录存在 (兼容 Windows/Linux)
void ensure_dir(const char *path) {
#ifdef _WIN32
    mkdir(path); // MinGW/MSYS2 下直接用 mkdir 即可
#else
    mkdir(path, 0777);
#endif
}

void generate_item() {
    char id[64], name[64], tile[32];
    int cost;

    printf("\n--- [新建物品与模型] ---\n");
    printf("请输入 ID (英文，如 windmill): "); fgets(id, sizeof(id), stdin); trim(id);
    printf("请输入名称 (如 风车): "); fgets(name, sizeof(name), stdin); trim(name);
    printf("请输入 Tile 类型 (如 house/road/tree): "); fgets(tile, sizeof(tile), stdin); trim(tile);
    printf("请输入花费资金: "); scanf("%d", &cost); getchar(); // 吸收回车

    // 1. 写入 ../items/<id>.json
    char json_path[256];
    snprintf(json_path, sizeof(json_path), "../items/%s.json", id);
    FILE *jf = fopen(json_path, "w");
    if (jf) {
        fprintf(jf, "{\n  \"id\": \"%s\",\n  \"name\": \"%s\",\n  \"tile\": \"%s\",\n  \"cost\": %d,\n  \"pixels\": [\n    \"..RRR..\",\n    \".RRRRR.\",\n    \"..BBB..\"\n  ]\n}\n", id, name, tile, cost);
        fclose(jf);
        printf("[成功] 已生成物品数据: assets/items/%s.json\n", id);
    } else {
        printf("[错误] 无法创建文件: %s\n", json_path);
    }

    // 2. 写入 ../scripts/models/<id>.lua
    char lua_path[256];
    snprintf(lua_path, sizeof(lua_path), "../scripts/models/%s.lua", id);
    FILE *lf = fopen(lua_path, "w");
    if (lf) {
        fprintf(lf, "-- 自动生成的模型: %s\n", name);
        fprintf(lf, "local function draw_%s(cr, sx, sy, zoom)\n", id);
        fprintf(lf, "    local tw = 64.0 * zoom\n    local th = 32.0 * zoom\n    local hw = tw / 2.0\n    local hh = th / 2.0\n    local height = 40.0 * zoom\n\n");
        fprintf(lf, "    -- 在此处编写你的 3D 绘制逻辑\n");
        fprintf(lf, "    set_rgb(cr, 0.8, 0.3, 0.3)\n");
        fprintf(lf, "    move_to(cr, sx, sy - height) line_to(cr, sx + hw, sy + hh - height) line_to(cr, sx, sy + th - height) line_to(cr, sx - hw, sy + hh - height) close_path(cr) fill(cr)\n");
        fprintf(lf, "end\n\nRegisterModel(\"%s\", draw_%s)\n", id, id);
        fclose(lf);
        printf("[成功] 已生成 3D 模型脚本: assets/scripts/models/%s.lua\n", id);
    } else {
        printf("[错误] 无法创建文件: %s\n", lua_path);
    }
}

void generate_event() {
    char id[64], title[64], desc[256];
    printf("\n--- [新建事件] ---\n");
    printf("请输入事件 ID (英文): "); fgets(id, sizeof(id), stdin); trim(id);
    printf("请输入事件标题: "); fgets(title, sizeof(title), stdin); trim(title);
    printf("请输入事件描述: "); fgets(desc, sizeof(desc), stdin); trim(desc);

    // 写入 ../events/<id>.json
    char path[256];
    snprintf(path, sizeof(path), "../events/%s.json", id);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "{\n  \"id\": \"%s\",\n  \"title\": \"%s\",\n  \"desc\": \"%s\",\n  \"probability\": 10,\n  \"condition\": { \"min_day\": 1 },\n  \"choices\": [\n    { \"text\": \"好的\", \"money\": 50, \"pop\": 10 }\n  ]\n}\n", id, title, desc);
        fclose(f);
        printf("[成功] 已生成事件配置: assets/events/%s.json\n", id);
    }
}

void generate_char() {
    char id[64], name[64];
    printf("\n--- [新建角色] ---\n");
    printf("请输入角色 ID (英文): "); fgets(id, sizeof(id), stdin); trim(id);
    printf("请输入角色名称: "); fgets(name, sizeof(name), stdin); trim(name);

    char path[256];
    snprintf(path, sizeof(path), "../chars/%s.json", id);
    FILE *f = fopen(path, "w");
    if (f) {
        fprintf(f, "{\n  \"id\": \"%s\",\n  \"name\": \"%s\",\n  \"portrait\": \"\",\n  \"honesty\": 50,\n  \"efficiency\": 50,\n  \"charisma\": 50\n}\n", id, name);
        fclose(f);
        printf("[成功] 已生成角色数据: assets/chars/%s.json\n", id);
    }
}

int main() {
    // 确保必要目录存在
    ensure_dir("../items");
    ensure_dir("../events");
    ensure_dir("../chars");
    ensure_dir("../scripts");
    ensure_dir("../scripts/models");
    ensure_dir("../story");

    int choice = 0;
    while (choice != 9) {
        printf("\n==================================\n");
        printf("   CityGtk4 官方模组开发套件 (CLI)\n");
        printf("==================================\n");
        printf(" 1. 新建物品 & 3D模型\n");
        printf(" 2. 新建随机事件\n");
        printf(" 3. 新建角色\n");
        printf(" 9. 退出\n");
        printf("请选择: ");
        scanf("%d", &choice);
        getchar(); // 吸收换行符

        switch (choice) {
            case 1: generate_item(); break;
            case 2: generate_event(); break;
            case 3: generate_char(); break;
            case 9: printf("拜拜！\n"); break;
            default: printf("无效选择！\n");
        }
    }
    return 0;
}