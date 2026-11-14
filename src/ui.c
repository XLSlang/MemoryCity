#include "ui.h"
#include "render.h"
#include "game.h"
#include "mouse.h"
#include "item/item.h"
#include "event/event.h"
#include "story/story.h"
#include "character/character.h"
#include "save/save.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static GtkWidget *lbl_population = NULL;
static GtkWidget *lbl_money      = NULL;
static GtkWidget *lbl_day        = NULL;
static GtkWidget *drawing_area   = NULL;
static GtkWidget *g_main_window  = NULL;
static GtkWidget *event_dialog   = NULL;

static int  g_current_slot = 1;
static bool g_game_started = false;

// ============================================================
// 选槽界面（全屏）
// ============================================================
typedef struct {
    GtkWidget *window;
    GtkApplication *app;
    GtkWidget *combo;
} SlotScreen;

// 前向声明
void ui_start_game(GtkApplication *app);

static void slot_screen_launch_game(SlotScreen *s, int slot, bool load) {
    g_current_slot = slot;

    // 1) 先初始化子系统（事件/物品/剧情/人物）
    event_init();
    item_init();
    story_init();
    character_init();

    // 2) 再初始化游戏世界
    game_init();

    // 3) 最后读档（覆盖世界与事件状态）
    if (load) {
        char *p = save_slot_path(slot);
        if (p) {
            game_load_from(p);
            g_free(p);
        }
    }

    // 4) 销毁选槽窗口，进入主界面
    gtk_window_destroy(GTK_WINDOW(s->window));
    g_free(s);

    ui_start_game(s->app);
}

static void on_slot_start_clicked(GtkButton *btn, gpointer data) {
    SlotScreen *s = (SlotScreen*)data;
    int slot = (int)gtk_drop_down_get_selected(GTK_DROP_DOWN(s->combo)) + 1;
    bool load = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(btn), "load")) != 0;
    slot_screen_launch_game(s, slot, load);
}

static gboolean on_slot_window_close(GtkWindow *win, gpointer data) {
    (void)win;
    SlotScreen *s = (SlotScreen*)data;
    g_application_quit(G_APPLICATION(s->app));
    return TRUE;
}

static void ui_show_slot_screen(GtkApplication *app) {
    SlotScreen *s = (SlotScreen*)calloc(1, sizeof(SlotScreen));
    s->app = app;

    GtkWidget *window = gtk_application_window_new(app);
    s->window = window;
    gtk_window_set_title(GTK_WINDOW(window), "MemoryCity - 选择存档");
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    gtk_window_fullscreen(GTK_WINDOW(window));

    GtkWidget *center = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_halign(center, GTK_ALIGN_CENTER);
    gtk_widget_set_valign(center, GTK_ALIGN_CENTER);
    gtk_widget_set_hexpand(center, TRUE);
    gtk_widget_set_vexpand(center, TRUE);

    GtkWidget *title = gtk_label_new("MemoryCity");
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(center), title);

    GtkWidget *sub = gtk_label_new("选择存档槽位");
    gtk_box_append(GTK_BOX(center), sub);

    GtkWidget *combo = gtk_drop_down_new_from_strings(
        (const char*[]){"槽位 1", "槽位 2", "槽位 3", NULL});
    s->combo = combo;
    gtk_widget_set_size_request(combo, 240, -1);
    gtk_box_append(GTK_BOX(center), combo);

    for (int i = 1; i <= SAVE_SLOT_COUNT; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "槽位 %d: %s", i,
                 save_slot_exists(i) ? "有存档" : "空");
        GtkWidget *l = gtk_label_new(buf);
        gtk_widget_set_halign(l, GTK_ALIGN_START);
        gtk_box_append(GTK_BOX(center), l);
    }

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_top(btn_box, 20);

    GtkWidget *btn_load = gtk_button_new_with_label("读取存档");
    GtkWidget *btn_new  = gtk_button_new_with_label("新游戏");
    gtk_widget_set_size_request(btn_load, 160, 48);
    gtk_widget_set_size_request(btn_new, 160, 48);

    g_object_set_data(G_OBJECT(btn_load), "load", GINT_TO_POINTER(1));
    g_object_set_data(G_OBJECT(btn_new),  "load", GINT_TO_POINTER(0));

    g_signal_connect(btn_load, "clicked", G_CALLBACK(on_slot_start_clicked), s);
    g_signal_connect(btn_new,  "clicked", G_CALLBACK(on_slot_start_clicked), s);

    gtk_box_append(GTK_BOX(btn_box), btn_load);
    gtk_box_append(GTK_BOX(btn_box), btn_new);
    gtk_box_append(GTK_BOX(center), btn_box);

    gtk_window_set_child(GTK_WINDOW(window), center);
    g_signal_connect(window, "close-request", G_CALLBACK(on_slot_window_close), s);

    gtk_window_present(GTK_WINDOW(window));
}

// ============================================================
// 主界面
// ============================================================
void ui_update_stats(void) {
    if (!lbl_day || !lbl_money || !lbl_population) return;
    GameState *state = game_get_state();
    char buf[64];
    snprintf(buf, sizeof(buf), "Day: %d", state->day);
    gtk_label_set_text(GTK_LABEL(lbl_day), buf);
    snprintf(buf, sizeof(buf), "Money: $%d", state->money);
    gtk_label_set_text(GTK_LABEL(lbl_money), buf);
    snprintf(buf, sizeof(buf), "Pop: %d / %d", state->population, state->pop_capacity);
    gtk_label_set_text(GTK_LABEL(lbl_population), buf);
}

static void on_event_choice(GtkButton *btn, gpointer data) {
    (void)btn;
    int idx = GPOINTER_TO_INT(data);
    event_apply_choice(idx);
    if (event_dialog) {
        gtk_window_destroy(GTK_WINDOW(event_dialog));
        event_dialog = NULL;
    }
    ui_update_stats();
}

static void show_event_dialog(void) {
    if (event_dialog) return;
    const EventDef *ev = event_current();
    if (!ev) return;

    event_dialog = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(event_dialog), ev->title);
    gtk_window_set_default_size(GTK_WINDOW(event_dialog), 480, 320);
    gtk_window_set_modal(GTK_WINDOW(event_dialog), TRUE);
    if (g_main_window)
        gtk_window_set_transient_for(GTK_WINDOW(event_dialog), GTK_WINDOW(g_main_window));

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *title = gtk_label_new(ev->title);
    gtk_widget_add_css_class(title, "title-1");
    gtk_box_append(GTK_BOX(box), title);

    GtkWidget *desc = gtk_label_new(ev->desc);
    gtk_label_set_wrap(GTK_LABEL(desc), TRUE);
    gtk_box_append(GTK_BOX(box), desc);

    for (int i = 0; i < ev->choice_count; i++) {
        GtkWidget *btn = gtk_button_new_with_label(ev->choice_text[i]);
        g_signal_connect(btn, "clicked", G_CALLBACK(on_event_choice), GINT_TO_POINTER(i));
        gtk_box_append(GTK_BOX(box), btn);
    }

    gtk_window_set_child(GTK_WINDOW(event_dialog), box);
    gtk_window_present(GTK_WINDOW(event_dialog));
}

static gboolean on_tick(gpointer data) {
    (void)data;
    if (!g_game_started) return G_SOURCE_REMOVE;
    game_tick();
    story_tick();
    if (!event_is_active()) event_try_trigger();
    if (event_is_active()) show_event_dialog();
    ui_update_stats();
    if (drawing_area) render_redraw(drawing_area);
    return G_SOURCE_CONTINUE;
}

static void on_tool_house(GtkButton *btn, gpointer data)    { (void)btn; (void)data; game_set_tool(TOOL_HOUSE); }
static void on_tool_road(GtkButton *btn, gpointer data)     { (void)btn; (void)data; game_set_tool(TOOL_ROAD); }
static void on_tool_bulldoze(GtkButton *btn, gpointer data) { (void)btn; (void)data; game_set_tool(TOOL_BULLDOZE); }

static void on_item_clicked(GtkButton *btn, gpointer data) {
    (void)data;
    const char *id = (const char*)g_object_get_data(G_OBJECT(btn), "item-id");
    if (!id) return;
    const ItemDef *it = item_find(id);
    if (!it) return;
    game_set_item(it->id, it->tile, it->cost);
    ui_update_stats();
}

static void on_item_preview_draw(GtkDrawingArea *area, cairo_t *cr,
                                 int width, int height, gpointer data) {
    (void)area;
    const ItemDef *it = (const ItemDef*)data;
    if (!it || it->width <= 0 || it->height <= 0) return;

    int maxdim = it->width > it->height ? it->width : it->height;
    double scale = 32.0 / (double)maxdim;
    if (scale < 0.5) scale = 0.5;

    double w = it->width  * scale;
    double h = it->height * scale;
    double ox = (width  - w) / 2.0;
    double oy = (height - h) / 2.0;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;
    item_render(cr, it, ox, oy, scale);
}

static GtkWidget* make_item_row(const ItemDef *it) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    GtkWidget *preview = gtk_drawing_area_new();
    gtk_widget_set_size_request(preview, 32, 32);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(preview),
                                   on_item_preview_draw,
                                   (gpointer)it, NULL);
    gtk_box_append(GTK_BOX(row), preview);

    char label[64];
    snprintf(label, sizeof(label), "%s $%d", it->name, it->cost);
    GtkWidget *btn = gtk_button_new_with_label(label);
    gtk_widget_set_hexpand(btn, TRUE);
    gtk_widget_set_halign(btn, GTK_ALIGN_FILL);

    GtkWidget *lbl = gtk_button_get_child(GTK_BUTTON(btn));
    if (lbl) gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);

    g_object_set_data_full(G_OBJECT(btn), "item-id", g_strdup(it->id), g_free);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_item_clicked), NULL);
    gtk_box_append(GTK_BOX(row), btn);
    return row;
}

static void on_save_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    char *p = save_slot_path(g_current_slot);
    if (p) { game_save_to(p); g_free(p); }
    ui_update_stats();
}

static void on_load_clicked(GtkButton *btn, gpointer data) {
    (void)btn; (void)data;
    char *p = save_slot_path(g_current_slot);
    if (p) {
        if (game_load_from(p)) {
            ui_update_stats();
            if (drawing_area) render_redraw(drawing_area);
        }
        g_free(p);
    }
}

// 退出确认
typedef struct {
    GtkWidget *dialog;
    GtkWindow *parent;
} CloseCtx;

static void close_dialog_response(GtkWidget *dialog, int response, gpointer user_data) {
    (void)dialog;
    CloseCtx *ctx = (CloseCtx*)user_data;
    if (response == GTK_RESPONSE_ACCEPT) {
        char *p = save_slot_path(g_current_slot);
        if (p) { game_save_to(p); g_free(p); }
        gtk_window_destroy(GTK_WINDOW(ctx->dialog));
        gtk_window_destroy(ctx->parent);
        g_free(ctx);
    } else if (response == GTK_RESPONSE_REJECT) {
        gtk_window_destroy(GTK_WINDOW(ctx->dialog));
        gtk_window_destroy(ctx->parent);
        g_free(ctx);
    } else {
        gtk_window_destroy(GTK_WINDOW(ctx->dialog));
        g_free(ctx);
    }
}

void close_btn_clicked(GtkButton *b, gpointer user_data) {
    CloseCtx *ctx = (CloseCtx*)user_data;
    int resp = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(b), "resp"));
    close_dialog_response(ctx->dialog, resp, ctx);
}

static gboolean on_close_request(GtkWindow *win, gpointer data) {
    (void)data;
    CloseCtx *ctx = (CloseCtx*)calloc(1, sizeof(CloseCtx));
    ctx->parent = win;

    GtkWidget *dialog = gtk_window_new();
    ctx->dialog = dialog;
    gtk_window_set_title(GTK_WINDOW(dialog), "退出前保存？");
    gtk_window_set_default_size(GTK_WINDOW(dialog), 320, 180);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_transient_for(GTK_WINDOW(dialog), win);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(box, 20);
    gtk_widget_set_margin_end(box, 20);
    gtk_widget_set_margin_top(box, 20);
    gtk_widget_set_margin_bottom(box, 20);

    GtkWidget *lbl = gtk_label_new("是否保存当前进度后退出？");
    gtk_widget_set_halign(lbl, GTK_ALIGN_START);
    gtk_box_append(GTK_BOX(box), lbl);

    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_halign(btn_box, GTK_ALIGN_END);
    GtkWidget *btn_save   = gtk_button_new_with_label("保存并退出");
    GtkWidget *btn_cancel = gtk_button_new_with_label("直接退出");
    GtkWidget *btn_back   = gtk_button_new_with_label("取消");
    gtk_box_append(GTK_BOX(btn_box), btn_save);
    gtk_box_append(GTK_BOX(btn_box), btn_cancel);
    gtk_box_append(GTK_BOX(btn_box), btn_back);
    gtk_box_append(GTK_BOX(box), btn_box);

    gtk_window_set_child(GTK_WINDOW(dialog), box);

    g_object_set_data(G_OBJECT(btn_save),   "resp", GINT_TO_POINTER(GTK_RESPONSE_ACCEPT));
    g_object_set_data(G_OBJECT(btn_cancel), "resp", GINT_TO_POINTER(GTK_RESPONSE_REJECT));
    g_object_set_data(G_OBJECT(btn_back),   "resp", GINT_TO_POINTER(GTK_RESPONSE_CANCEL));

    g_signal_connect(btn_save,   "clicked", G_CALLBACK(close_btn_clicked), ctx);
    g_signal_connect(btn_cancel, "clicked", G_CALLBACK(close_btn_clicked), ctx);
    g_signal_connect(btn_back,   "clicked", G_CALLBACK(close_btn_clicked), ctx);

    gtk_window_present(GTK_WINDOW(dialog));
    return TRUE;
}

void ui_start_game(GtkApplication *app) {
    g_game_started = true;

    GtkWidget *window = gtk_application_window_new(app);
    g_main_window = window;
    gtk_window_set_title(GTK_WINDOW(window), "MemoryCity");
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    GtkWidget *topbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 20);
    gtk_widget_set_name(topbar, "topbar");
    gtk_widget_set_margin_top(topbar, 10);
    gtk_widget_set_margin_bottom(topbar, 10);
    gtk_widget_set_margin_start(topbar, 20);

    lbl_day        = gtk_label_new("Day: 1");
    lbl_money      = gtk_label_new("Money: $1000");
    lbl_population = gtk_label_new("Pop: 0 / 0");
    gtk_box_append(GTK_BOX(topbar), lbl_day);
    gtk_box_append(GTK_BOX(topbar), lbl_money);
    gtk_box_append(GTK_BOX(topbar), lbl_population);
    gtk_box_append(GTK_BOX(main_vbox), topbar);

    GtkWidget *sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_top(sidebar, 8);
    gtk_widget_set_margin_bottom(sidebar, 8);
    gtk_widget_set_margin_start(sidebar, 8);
    gtk_widget_set_margin_end(sidebar, 8);

    GtkWidget *lbl_tools = gtk_label_new("建造工具");
    gtk_widget_set_halign(lbl_tools, GTK_ALIGN_START);
    gtk_widget_add_css_class(lbl_tools, "title-4");
    gtk_box_append(GTK_BOX(sidebar), lbl_tools);

    GtkWidget *btn_house    = gtk_button_new_with_label("住宅 $100");
    GtkWidget *btn_road     = gtk_button_new_with_label("道路 $10");
    GtkWidget *btn_bulldoze = gtk_button_new_with_label("拆除");
    g_signal_connect(btn_house,    "clicked", G_CALLBACK(on_tool_house),    NULL);
    g_signal_connect(btn_road,     "clicked", G_CALLBACK(on_tool_road),     NULL);
    g_signal_connect(btn_bulldoze, "clicked", G_CALLBACK(on_tool_bulldoze), NULL);
    gtk_box_append(GTK_BOX(sidebar), btn_house);
    gtk_box_append(GTK_BOX(sidebar), btn_road);
    gtk_box_append(GTK_BOX(sidebar), btn_bulldoze);

    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *lbl_items = gtk_label_new("物品");
    gtk_widget_set_halign(lbl_items, GTK_ALIGN_START);
    gtk_widget_add_css_class(lbl_items, "title-4");
    gtk_box_append(GTK_BOX(sidebar), lbl_items);

    for (int i = 0; i < item_count(); i++) {
        const ItemDef *it = item_get(i);
        if (!it) continue;
        gtk_box_append(GTK_BOX(sidebar), make_item_row(it));
    }

    gtk_box_append(GTK_BOX(sidebar), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));

    GtkWidget *btn_save = gtk_button_new_with_label("保存到当前槽");
    GtkWidget *btn_load = gtk_button_new_with_label("读取当前槽");
    g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), NULL);
    g_signal_connect(btn_load, "clicked", G_CALLBACK(on_load_clicked), NULL);
    gtk_box_append(GTK_BOX(sidebar), btn_save);
    gtk_box_append(GTK_BOX(sidebar), btn_load);

    GtkWidget *sidebar_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scroll), sidebar);
    gtk_widget_set_size_request(sidebar_scroll, 180, -1);

    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_vexpand(drawing_area, TRUE);
    gtk_widget_set_hexpand(drawing_area, TRUE);
    render_init(GTK_DRAWING_AREA(drawing_area));

    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(paned, TRUE);
    gtk_paned_set_start_child(GTK_PANED(paned), sidebar_scroll);
    gtk_paned_set_end_child(GTK_PANED(paned), drawing_area);
    gtk_paned_set_resize_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_shrink_start_child(GTK_PANED(paned), FALSE);
    gtk_paned_set_position(GTK_PANED(paned), 180);
    gtk_box_append(GTK_BOX(main_vbox), paned);

    gtk_window_set_child(GTK_WINDOW(window), main_vbox);
    mouse_attach_to_canvas(drawing_area);

    GtkCssProvider *css = gtk_css_provider_new();
    if (g_file_test("assets/style.css", G_FILE_TEST_EXISTS)) {
        gtk_css_provider_load_from_path(css, "assets/style.css");
    }
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_timeout_add(1000, on_tick, NULL);
    g_signal_connect(window, "close-request", G_CALLBACK(on_close_request), NULL);

    ui_update_stats();
    gtk_window_present(GTK_WINDOW(window));
    gtk_paned_set_position(GTK_PANED(paned), 180);
}

void ui_start(GtkApplication *app) {
    ui_show_slot_screen(app);
}