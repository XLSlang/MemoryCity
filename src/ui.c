#include "ui.h"
#include "render.h"
#include "game.h"
#include "mouse.h"
#include "item/item.h"
#include "event/event.h"
#include "story/story.h"
#include "character/character.h"
#include <stdio.h>
#include <string.h>

static GtkWidget *lbl_population = NULL;
static GtkWidget *lbl_money      = NULL;
static GtkWidget *lbl_day        = NULL;
static GtkWidget *drawing_area   = NULL;
static GtkWidget *g_main_window  = NULL;

static GtkWidget *event_dialog = NULL;

// ---------- 状态栏 ----------
void ui_update_stats(void) {
    GameState *state = game_get_state();
    char buf[64];
    snprintf(buf, sizeof(buf), "Day: %d", state->day);
    gtk_label_set_text(GTK_LABEL(lbl_day), buf);
    snprintf(buf, sizeof(buf), "Money: $%d", state->money);
    gtk_label_set_text(GTK_LABEL(lbl_money), buf);
    snprintf(buf, sizeof(buf), "Pop: %d / %d", state->population, state->pop_capacity);
    gtk_label_set_text(GTK_LABEL(lbl_population), buf);
}

// ---------- 事件弹窗 ----------
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

// ---------- 每秒 tick ----------
static gboolean on_tick(gpointer data) {
    (void)data;

    game_tick();
    story_tick();

    if (!event_is_active())
        event_try_trigger();

    if (event_is_active())
        show_event_dialog();

    ui_update_stats();
    render_redraw(drawing_area);
    return G_SOURCE_CONTINUE;
}

// ---------- 工具栏回调 ----------
static void on_tool_house(GtkButton *btn, gpointer data)    { (void)btn; (void)data; game_set_tool(TOOL_HOUSE); }
static void on_tool_road(GtkButton *btn, gpointer data)     { (void)btn; (void)data; game_set_tool(TOOL_ROAD); }
static void on_tool_bulldoze(GtkButton *btn, gpointer data) { (void)btn; (void)data; game_set_tool(TOOL_BULLDOZE); }

// ---------- 物品栏回调 ----------
static void on_item_clicked(GtkButton *btn, gpointer data) {
    (void)data;
    const char *id = (const char*)g_object_get_data(G_OBJECT(btn), "item-id");
    if (!id) return;
    const ItemDef *it = item_find(id);
    if (!it) return;
    game_set_item(it->id, it->tile, it->cost);
    ui_update_stats();
}

// 为物品生成一个小预览图：按物品实际尺寸自适应缩放居中
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

// 构建单行物品控件：预览 + 名称 + 花费
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

    // 长名称截断
    GtkWidget *lbl = gtk_button_get_child(GTK_BUTTON(btn));
    if (lbl) gtk_label_set_ellipsize(GTK_LABEL(lbl), PANGO_ELLIPSIZE_END);

    g_object_set_data_full(G_OBJECT(btn), "item-id", g_strdup(it->id), g_free);
    g_signal_connect(btn, "clicked", G_CALLBACK(on_item_clicked), NULL);
    gtk_box_append(GTK_BOX(row), btn);
    return row;
}

// ---------- 入口 ----------
void ui_start(GtkApplication *app) {
    event_init();
    item_init();
    story_init();
    character_init();

    GtkWidget *window = gtk_application_window_new(app);
    g_main_window = window;
    gtk_window_set_title(GTK_WINDOW(window), "CityGtk4 - 2.5D");
    gtk_window_set_default_size(GTK_WINDOW(window), 1024, 768);

    GtkWidget *main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

    // 顶部状态栏
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

    // 侧边栏内容
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

    // 侧边栏滚动窗口
    GtkWidget *sidebar_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sidebar_scroll),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(sidebar_scroll), sidebar);
    gtk_widget_set_size_request(sidebar_scroll, 180, -1);

    // 画布
    drawing_area = gtk_drawing_area_new();
    gtk_widget_set_vexpand(drawing_area, TRUE);
    gtk_widget_set_hexpand(drawing_area, TRUE);
    render_init(GTK_DRAWING_AREA(drawing_area));

    // 用 Paned 分成左右两栏，中间分隔条可拖拽
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
    } else {
        g_warning("CSS 未找到: assets/style.css");
    }
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(css),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    g_timeout_add(1000, on_tick, NULL);

    gtk_window_present(GTK_WINDOW(window));

    // present 之后再设一次，确保初始宽度生效
    gtk_paned_set_position(GTK_PANED(paned), 180);
}