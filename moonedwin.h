#ifndef __MOONEDWIN_H
#define __MOONEDWIN_H

#include <gtk/gtk.h>

#define MOONED_TYPE_WINDOW mooned_window_get_type ()
G_DECLARE_FINAL_TYPE (MoonedWindow, mooned_window, MOONED, WINDOW, GtkApplicationWindow)

#include "moonedapp.h"

MoonedWindow           *mooned_window_new          (MoonedApplication *app);
MoonedWindow           *mooned_window_open         (MoonedApplication *app, GFile *file);
void                    mooned_window_read         (MoonedWindow *win, GFile *file);
GFile                  *mooned_window_get_file     (MoonedWindow *win);
gboolean                mooned_window_is_empty     (MoonedWindow *win);
#endif /* __MOONEDWIN_H */
