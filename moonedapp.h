#ifndef __MOONEDAPP_H
#define __MOONEDAPP_H

#include <gtk/gtk.h>

#define MOONED_TYPE_APPLICATION mooned_application_get_type ()
G_DECLARE_FINAL_TYPE(MoonedApplication, mooned_application, MOONED, APPLICATION, GtkApplication)

/*moonedwin.hの中でMoonedApplicationを参照しているので、G_DECLARE_FINAL_TYPEの後でインクルード*/
#include "moonedwin.h"

MoonedApplication *mooned_application_new(void);
MoonedWindow *mooned_find_window_has_same_file(MoonedApplication *app, GFile *file);

#endif /* __MOONEDAPP_H */

