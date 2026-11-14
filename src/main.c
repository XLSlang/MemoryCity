#include <gtk/gtk.h>
#include "game.h"
#include "ui.h"
#include "dsl_lua.h"

static void activate(GtkApplication *app, gpointer user_data) {
    (void)user_data;
    dsl_lua_init();   // Lua 引擎先初始化，渲染依赖它
    ui_start(app);    // 只显示选槽界面
}

int main(int argc, char **argv) {
    GtkApplication *app = gtk_application_new(
        "org.example.citygtk4", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}