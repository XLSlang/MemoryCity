#include <gtk/gtk.h>
#include "game.h"
#include "ui.h"
#include "dsl_lua.h"    // 确保包含的是这个头文件

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    game_init();
    dsl_lua_init();     // 改这里，加上 dsl_ 前缀
    ui_start(app);
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new("org.example.citygtk4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}