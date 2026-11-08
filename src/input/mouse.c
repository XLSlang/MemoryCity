#include "mouse.h"
#include "render.h"
#include "game.h"
#include "ui.h"

static double cam_start_x, cam_start_y;
static int    drag_button = 0;
static double drag_total = 0.0;   // 累计拖动距离，用于区分"单击"和"拖拽"

// 左键：建造
static void on_click_left(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
    (void)n_press; (void)data;
    int mx, my; bool valid;
    render_mouse_to_grid((int)x, (int)y, &mx, &my, &valid);
    if (valid) {
        game_handle_click(mx, my, 1);
        ui_update_stats();
        gtk_widget_queue_draw(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture))));
    }
}

// 右键单击：拆除（若刚拖拽过则忽略）
static void on_click_right(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data) {
    (void)n_press; (void)data;
    if (drag_total > 5.0) { drag_total = 0.0; return; }  // 拖动过，不拆除
    int mx, my; bool valid;
    render_mouse_to_grid((int)x, (int)y, &mx, &my, &valid);
    if (valid) {
        game_handle_click(mx, my, 3);
        ui_update_stats();
        gtk_widget_queue_draw(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture))));
    }
}

// 滚轮缩放
static gboolean on_scroll(GtkEventControllerScroll *controller, double dx, double dy, gpointer data) {
    (void)dx; (void)data;
    GameState *state = game_get_state();
    double new_zoom = state->cam_zoom + ((dy > 0) ? -0.1 : 0.1);
    if (new_zoom >= 0.3 && new_zoom <= 3.0) {
        state->cam_zoom = new_zoom;
        gtk_widget_queue_draw(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller))));
    }
    return TRUE;
}

// 右键拖拽平移
static void on_drag_begin(GtkGestureDrag *gesture, double start_x, double start_y, gpointer data) {
    (void)data; (void)start_x; (void)start_y;
    drag_button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));
    if (drag_button != 3) return;

    drag_total = 0.0;
    GameState *state = game_get_state();
    cam_start_x = state->cam_offset_x;
    cam_start_y = state->cam_offset_y;
}

static void on_drag_update(GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer data) {
    (void)data;
    if (drag_button != 3) return;

    drag_total += (offset_x > 0 ? offset_x : -offset_x) + (offset_y > 0 ? offset_y : -offset_y);

    GameState *state = game_get_state();
    state->cam_offset_x = cam_start_x + offset_x;
    state->cam_offset_y = cam_start_y + offset_y;
    gtk_widget_queue_draw(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(gesture))));
}

static void on_drag_end(GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer data) {
    (void)gesture; (void)offset_x; (void)offset_y; (void)data;
    // drag_total 在 click 回调里消费后清零
}

// 鼠标悬停
static void on_motion(GtkEventControllerMotion *controller, double x, double y, gpointer data) {
    (void)data;
    int mx, my; bool valid;
    render_mouse_to_grid((int)x, (int)y, &mx, &my, &valid);
    GameState *state = game_get_state();

    if (valid) {
        if (!state->has_hover || state->hover_x != mx || state->hover_y != my) {
            state->hover_x = mx; state->hover_y = my; state->has_hover = true;
            gtk_widget_queue_draw(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller))));
        }
    } else if (state->has_hover) {
        state->has_hover = false;
        gtk_widget_queue_draw(GTK_WIDGET(gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(controller))));
    }
}

void mouse_attach_to_canvas(GtkWidget *canvas) {
    // 左键单击
    GtkGesture *click_l = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_l), 1);
    g_signal_connect(click_l, "pressed", G_CALLBACK(on_click_left), NULL);
    gtk_widget_add_controller(canvas, GTK_EVENT_CONTROLLER(click_l));

    // 右键单击（拆除）
    GtkGesture *click_r = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click_r), 3);
    g_signal_connect(click_r, "pressed", G_CALLBACK(on_click_right), NULL);
    gtk_widget_add_controller(canvas, GTK_EVENT_CONTROLLER(click_r));

    // 悬停
    GtkEventController *motion = gtk_event_controller_motion_new();
    g_signal_connect(motion, "motion", G_CALLBACK(on_motion), NULL);
    gtk_widget_add_controller(canvas, motion);

    // 滚轮缩放
    GtkEventController *scroll = gtk_event_controller_scroll_new(GTK_EVENT_CONTROLLER_SCROLL_VERTICAL);
    g_signal_connect(scroll, "scroll", G_CALLBACK(on_scroll), NULL);
    gtk_widget_add_controller(canvas, scroll);

    // 右键拖拽平移
    GtkGesture *drag = gtk_gesture_drag_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(drag), 3);
    g_signal_connect(drag, "drag-begin",  G_CALLBACK(on_drag_begin),  NULL);
    g_signal_connect(drag, "drag-update", G_CALLBACK(on_drag_update), NULL);
    g_signal_connect(drag, "drag-end",    G_CALLBACK(on_drag_end),    NULL);
    gtk_widget_add_controller(canvas, GTK_EVENT_CONTROLLER(drag));
}