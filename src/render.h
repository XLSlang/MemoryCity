#ifndef RENDER_H
#define RENDER_H

#include <gtk/gtk.h>

void render_init(GtkDrawingArea *area);
void render_mouse_to_grid(int screen_x, int screen_y, int *mx, int *my, bool *is_valid);
void render_redraw(GtkWidget *drawing_area);

#endif // RENDER_H