#include <string.h>
#include "moonedapp.h"
#include "moonedwin.h"

struct _MoonedApplication {
  GtkApplication parent;
};

G_DEFINE_TYPE(MoonedApplication, mooned_application, GTK_TYPE_APPLICATION);

/*--------------------------------------------*/
/*ヘルパー（このファイル内からのみ参照される）*/
/*--------------------------------------------*/

static GFile *
mooned_open_file_chooser_dialog(void) {
  GtkFileChooser *dialog;
  gint res;
  GFile *file;

  dialog = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Open File",  NULL, GTK_FILE_CHOOSER_ACTION_OPEN,
                                      "Cancel", GTK_RESPONSE_CANCEL,
                                      "Open", GTK_RESPONSE_ACCEPT,
                                      NULL));
  res = gtk_dialog_run(GTK_DIALOG(dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    file = gtk_file_chooser_get_file(dialog);
  else /* res == GTK_RESPONSE_CANCEL */
    file = NULL;
  gtk_widget_destroy(GTK_WIDGET(dialog));
  return file;
}

/*宣言だけ。定義はファイルの最後の方に*/
/*fileを編集中であるウィンドウを返す。なければNULLを返す*/
MoonedWindow *
mooned_find_window_has_same_file(MoonedApplication *app, GFile *file);

/*空のウィンドウがあるかどうかのチェック*/
/*空のウィンドウがある　＝＞　そのウィンドウを返す*/
/*空のウィンドウはない　＝＞　NULLを返す*/
static MoonedWindow *
mooned_find_empty_window(MoonedApplication *app) {
  GList *windows;
  MoonedWindow *win;

  for (windows = gtk_application_get_windows(GTK_APPLICATION(app)); windows != NULL; windows = windows->next) {
    win = MOONED_WINDOW(windows->data);
    if (mooned_window_is_empty(win))
      return win;
  }
  return NULL;
}

/*--------------------------------------------*/
/*アクションのアクティベートシグナルのハンドラ*/
/*     メニュー＝＞アクション＝＞ハンドラ     */
/*--------------------------------------------*/

static void
new_activated(GSimpleAction *action, GVariant *parameter, gpointer app) {
  MoonedWindow *win;

  win = mooned_window_new(MOONED_APPLICATION(app));
  gtk_window_present(GTK_WINDOW(win));
}

static void
open_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedApplication *app = user_data;
  GFile *file;
  MoonedWindow *win, *same_win, *empty_win;

  if ((file = mooned_open_file_chooser_dialog()) == NULL) /*Cancel*/
    return;
  same_win = mooned_find_window_has_same_file(app, file);
  empty_win = mooned_find_empty_window(app);
  if (same_win)
    gtk_window_present(GTK_WINDOW(same_win));
  else if(empty_win) {
    mooned_window_read(empty_win, file);
    gtk_window_present(GTK_WINDOW(empty_win));
  }else {
    win = mooned_window_open(app, file);
    gtk_window_present(GTK_WINDOW(win));
  }
  g_object_unref(file);
}

static void
quit_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedApplication *app = user_data;
/*  g_application_quit(G_APPLICATION(app));*/
/*このquitの関数を使うと、ウィンドウにdestroyシグナルが届かずに終了となってしまう*/
/*destroyシグナルのハンドラで、各ウィンドウの未保存のバッファを保存したいので、下記のように変更した*/

  GList *windows;

/* GListをコピーしておかないと、close(によるdestroy)したときに、リストが変わってしまう */
  for (windows = g_list_copy(gtk_application_get_windows(GTK_APPLICATION(app))); windows != NULL; windows = windows->next)
    g_action_activate(g_action_map_lookup_action(G_ACTION_MAP(windows->data), "close"), NULL); /*closeアクションをアクティベート*/
}

/*--------------------------------------------*/
/* クラスのオブジェクト・メソッド・ハンドラの */
/* オーバーライド関数                         */
/*--------------------------------------------*/

static void
mooned_application_activate(GApplication *app) {
  MoonedWindow *win;

  win = mooned_window_new(MOONED_APPLICATION(app));
  gtk_window_present(GTK_WINDOW(win));
}

static void
mooned_application_open(GApplication *app, GFile **files, gint n_files, const gchar *hint) {
  int i;
  MoonedWindow *win;

  for (i = 0; i < n_files; i++) {
    win = mooned_find_window_has_same_file(MOONED_APPLICATION(app), files[i]);
    if (win == NULL)
      win = mooned_window_open(MOONED_APPLICATION(app), files[i]);
    gtk_window_present(GTK_WINDOW(win));
  }
}

/*アクションの定義*/
const GActionEntry app_entries[] = {
  {"new", new_activated, NULL, NULL, NULL},
  {"open", open_activated, NULL, NULL, NULL},
  {"quit", quit_activated, NULL, NULL, NULL}
};

/*アクセラレータの定義*/
struct {
  const gchar *action;
  const gchar *accels[2];
} action_accels[] = {
  { "app.new", { "<Control>n", NULL } },
  { "app.open", { "<Control>o", NULL } },
  { "app.quit", { "<Control>q", NULL } },
  { "win.save", { "<Control>s", NULL } },
  { "win.saveas", { "<Shift><Control>s", NULL } },
  { "win.close", { "<Control>w", NULL } },
  { "win.cut", { "<Control>x", NULL } },
  { "win.copy", { "<Control>c", NULL } },
  { "win.paste", { "<Control>v", NULL } },
  { "win.selectall", { "<Control>a", NULL } }
};

static void
mooned_application_startup(GApplication *app) {
  GtkBuilder *builder;
  GMenu *appmenu, *menubar;
  int i;

  G_APPLICATION_CLASS(mooned_application_parent_class)->startup(app);

  g_action_map_add_action_entries(G_ACTION_MAP(app), app_entries, G_N_ELEMENTS(app_entries), app);
  for (i = 0; i < G_N_ELEMENTS(action_accels); i++)
    gtk_application_set_accels_for_action(GTK_APPLICATION(app), action_accels[i].action, action_accels[i].accels);
  builder = gtk_builder_new_from_resource("/com/github/ToshioCP/Mooned/menu.ui");
  appmenu = G_MENU(gtk_builder_get_object(builder, "appmenu"));
  gtk_application_set_app_menu(GTK_APPLICATION(app), G_MENU_MODEL(appmenu));
  g_object_unref(appmenu);
  menubar = G_MENU(gtk_builder_get_object(builder, "menubar"));
  gtk_application_set_menubar(GTK_APPLICATION(app), G_MENU_MODEL(menubar));
  g_object_unref(menubar);
}

/*--------------------------------------------*/
/*        オブジェクトとクラスの初期化        */
/*--------------------------------------------*/

static void
mooned_application_init(MoonedApplication *app) {
}

static void
mooned_application_class_init(MoonedApplicationClass *class) {
  G_APPLICATION_CLASS(class)->activate = mooned_application_activate;
  G_APPLICATION_CLASS(class)->open = mooned_application_open;
  G_APPLICATION_CLASS(class)->startup = mooned_application_startup;
}

/*--------------------------------------------*/
/*             外部に公開する関数             */
/*--------------------------------------------*/

MoonedApplication *
mooned_application_new(void) {
  return g_object_new(MOONED_TYPE_APPLICATION,
                       "application-id", "com.github.ToshioCP.Mooned",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

/*ファイルを新しいウィンドウで開くときに、既存のウィンドウと同じファイルを開くのはまずい。*/
/*そこで、そのファイルのファイル名と同じものが既存のウィンドウのファイル名にあればそのウィンドウを返す*/
/*同じものがなければNULLを返す。つまり、find_window_has_file*/
/*この関数はmoonedwin.cでも保存時のチェックに使う*/
MoonedWindow *
mooned_find_window_has_same_file(MoonedApplication *app, GFile *file) {
  GList *windows;
  GFile *wfile;

  for (windows = gtk_application_get_windows(GTK_APPLICATION(app)); windows != NULL; windows = windows->next) {
    /*wfileをg_object_unref()してはいけない*/
    if ((wfile = mooned_window_get_file(MOONED_WINDOW(windows->data))) != NULL && g_file_equal(file, wfile))
      return MOONED_WINDOW(windows->data); /*見つかった*/
  }
  return NULL; /*結局見つからなかった*/
}

