#include <string.h>
#include "moonedapp.h"
#include "moonedwin.h"

/*
MoonedWindowは次のような特徴をもったウィジェットである。
1 GtkWindowの子オブジェクトである
 - ActionMapである＝＞アクションを登録できる
（注意）アクションはメニューに対応していて、例えば「Close」メニューがクリックされると対応しているcloseアクションがアクティブになる
closeアクションがアクティブになると、そのハンドラが起動され、Closeの処理が行われる。
2 GtkScrolledWindow（子）とGtkTextView（孫）を子孫ウィジェットにもつ
（注意）オブジェクトの親子関係とウィジェットの親子関係は別の概念である。混同しないように。
3 ウィンドウはファイルと1対1に対応する。ただし、ファイルと未対応のものもある（後で対応する予定のものである）。
このことは、「Open」「Save」「SaveAs」のメニューなどで、1対1対応が崩れないような注意が必要になる。
（注意）ファイルを表現するにはGFileへのポインタを使う。とくに4で述べるパブリック関数の引数や返し値はそれを使う。
4 外のオブジェクトからアクセスするためのパブリック関数を持っている

生成はmooned_window_newで。初期化もこの関数の中で行う
クローズはdelete_event_activatedまたはclose_activatedで行う
*/

struct _MoonedWindow
{
  GtkApplicationWindow parent;
  /* private data */
  GtkScrolledWindow *scrolled_window;
  GtkTextView *text_view;
  GtkTextBuffer *text_buffer;
  GFile *file;
  gboolean changed; /*テキストが書き換えられたらTRUE*/
};

G_DEFINE_TYPE(MoonedWindow, mooned_window, GTK_TYPE_APPLICATION_WINDOW);

/*他のルーチンから使われるヘルパー*/

/*「名前を変えて保存」のときのファイル選択ダイアログ*/

static GFile*
mooned_saveas_file_chooser_dialog(MoonedWindow *win) {
  GtkFileChooser *dialog;
  gint res;
  GFile *file;

  dialog = GTK_FILE_CHOOSER(gtk_file_chooser_dialog_new("Save as File", GTK_WINDOW(win), GTK_FILE_CHOOSER_ACTION_SAVE,
                                      "_Cancel", GTK_RESPONSE_CANCEL,
                                      "_Save as", GTK_RESPONSE_ACCEPT,
                                      NULL));
  res = gtk_dialog_run(GTK_DIALOG(dialog));
  if (res == GTK_RESPONSE_ACCEPT)
    file = gtk_file_chooser_get_file(dialog);
  else /* res == GTK_RESPONSE_CANCEL */
    file = NULL;
  gtk_widget_destroy(GTK_WIDGET(dialog));
  return file;
}

/*閉じる前に「保存しますか？」*/
/* Cancel => return TRUE, Save|Quit(not save) => return FALSE */

static gboolean
saveornot_before_close(MoonedWindow *win) {
  GtkWidget *message_dialog;
  gchar *filename;
  gint res;

  if (!(win->changed))
    return FALSE;
  filename = win->file ? g_file_get_basename(win->file) : "Untitled";
  message_dialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
                      GTK_BUTTONS_NONE, "Save changes to document %s before closing?", filename);
  gtk_dialog_add_buttons (GTK_DIALOG(message_dialog), "Close _without Saving", GTK_RESPONSE_REJECT,
                                                      "_Cancel", GTK_RESPONSE_CANCEL, "_Save", GTK_RESPONSE_ACCEPT,  NULL);
  if (win->file)
    g_free(filename);
  res = gtk_dialog_run(GTK_DIALOG(message_dialog));
  gtk_widget_destroy(message_dialog);
  switch (res) {
    case GTK_RESPONSE_ACCEPT:
      if (win->file)
        g_action_group_activate_action(G_ACTION_GROUP(win), "save", NULL);
      else
        g_action_group_activate_action(G_ACTION_GROUP(win), "saveas", NULL);
      return FALSE;
    case GTK_RESPONSE_REJECT:
      return FALSE;
    case GTK_RESPONSE_CANCEL:
      return TRUE;
    default: /*close bottun was pressed*/
      g_print("The bottun was not pressed.");
      return TRUE;
  }
}

/*シグナルに対するハンドラ*/

/*ウィンドウのタイトルバーのクローズボタンが押された時・・・これはXウィンドウの管理でGtkではないらしい*/

static gboolean
delete_event_activated(GtkWidget *object, GdkEvent *event, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);

  return saveornot_before_close(win);
}

/*バッファが書き換えられた時のハンドラ*/

static void
changed_activated(GtkTextBuffer *buffer, MoonedWindow *win){
  gchar *filename;
  gchar *asterisk_filename;

  if (win->changed)
    return;
  win->changed = TRUE;
  if (win->file) {
    filename = g_file_get_basename(win->file);
    asterisk_filename = g_strconcat ("*",filename, NULL);
    gtk_window_set_title(GTK_WINDOW(win), asterisk_filename);
    g_free(filename);
    g_free(asterisk_filename);
  }
}

/*save, saveasのヘルパー*/

/*ファイルが他のウィンドウと重なっていないかのテスト*/
/*TRUE => ファイルの重なりはない*/
/*FALSE => ファイルが重なっている（コンフリクト） => 警告メッセージを出す*/

static gboolean
file_unique_test(MoonedWindow *win, GFile *file) {
  MoonedApplication *app;
  MoonedWindow *same_win;
  GtkWidget *message_dialog;
  gchar *filename;

  app = MOONED_APPLICATION(gtk_window_get_application(GTK_WINDOW(win)));
  if ((same_win = mooned_find_window_containing_file(app, file))  && (same_win != win)) { /*conflict !*/
    filename = g_file_get_basename(file);
    message_dialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,
                      GTK_BUTTONS_CLOSE, "Can't save %s because it's in the other window.", filename);
    gtk_dialog_run(GTK_DIALOG(message_dialog));
    g_free(filename);
    gtk_widget_destroy(message_dialog);
    return FALSE;
  }else
    return TRUE;
}

/*バッファを（チェック済みの）ファイルに保存しタイトルのファイル名やchangedフラグを更新する*/
/*前提として、win->fileが更新されていて、そのファイルに保存する*/
static void
save_buffer(MoonedWindow *win) {
  GtkWidget *message_dialog;
  char *contents;
  gchar *filename;
  GtkTextIter start_iter;
  GtkTextIter end_iter;

  gtk_text_buffer_get_bounds(win->text_buffer, &start_iter, &end_iter);
  contents = gtk_text_buffer_get_text(win->text_buffer, &start_iter, &end_iter, TRUE);
  if (g_file_replace_contents(win->file, contents, strlen(contents), NULL, TRUE, G_FILE_CREATE_NONE, NULL, NULL, NULL)) {
    win->changed = FALSE;
    filename = g_file_get_basename(win->file);
    gtk_window_set_title(GTK_WINDOW(win), filename);
    g_free(filename);
  }else {
    filename = g_file_get_basename(win->file);
    message_dialog = gtk_message_dialog_new(GTK_WINDOW(win), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                    GTK_BUTTONS_CLOSE, "ERROR : Can't save %s.", filename);
    gtk_dialog_run(GTK_DIALOG(message_dialog));
    g_free(filename);
    gtk_widget_destroy(message_dialog);
  }
  g_free(contents);
}

/*メニュー＝＞アクション＝＞ハンドラ　の　各ハンドラ*/

static void
save_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);
  GFile *file;

  if (win->file == NULL) {
    if ((file = mooned_saveas_file_chooser_dialog(win)) == NULL)
      return; /* do nothing */
    if (! file_unique_test(win, file))
      return;
    win->file = file;
  }
  save_buffer(win);
}

static void
saveas_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);
  GFile *file;

  if (! (file = mooned_saveas_file_chooser_dialog(win)))
    return; /* do nothing */
  if (! file_unique_test(win, file))
    return;
  win->file = file;
  save_buffer(win);
}

static void
close_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);

  if (!(saveornot_before_close(win))) /*delete-eventとの関係でCancel=>TRUE, Save/Quit=>FALSEなので注意*/
    gtk_widget_destroy(GTK_WIDGET(win));
}

static void
cut_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);
  GtkClipboard *clipboard;

  clipboard = gtk_widget_get_clipboard(GTK_WIDGET(win->text_view), GDK_SELECTION_CLIPBOARD);
  gtk_text_buffer_cut_clipboard(win->text_buffer, clipboard, gtk_text_view_get_editable(win->text_view));
  gtk_text_view_scroll_mark_onscreen(win->text_view, gtk_text_buffer_get_insert(win->text_buffer));
}

static void
copy_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);
  GtkClipboard *clipboard;

  clipboard = gtk_widget_get_clipboard(GTK_WIDGET(win->text_view), GDK_SELECTION_CLIPBOARD);
  gtk_text_buffer_copy_clipboard(win->text_buffer, clipboard);
 }

static void
paste_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);
  GtkClipboard *clipboard;

  clipboard = gtk_widget_get_clipboard(GTK_WIDGET(win->text_view), GDK_SELECTION_CLIPBOARD);
  gtk_text_buffer_paste_clipboard(win->text_buffer, clipboard, NULL, gtk_text_view_get_editable(win->text_view));
}

static void
selectall_activated(GSimpleAction *action, GVariant *parameter, gpointer user_data) {
  MoonedWindow *win = MOONED_WINDOW(user_data);
  GtkTextIter start;
  GtkTextIter end;

  gtk_text_buffer_get_bounds(win->text_buffer, &start, &end);
  gtk_text_buffer_select_range(win->text_buffer, &start, &end);
}

/*オブジェクトとクラスの初期化*/

static void
mooned_window_init (MoonedWindow *win)
{
  gtk_widget_init_template(GTK_WIDGET(win));
  win->file = NULL;
}

static void
mooned_window_class_init (MoonedWindowClass *class)
{
  gtk_widget_class_set_template_from_resource(GTK_WIDGET_CLASS(class), "/com/github/ToshioCP/Mooned/moonedwin.ui");
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MoonedWindow, scrolled_window);
  gtk_widget_class_bind_template_child(GTK_WIDGET_CLASS(class), MoonedWindow, text_view);
}

const GActionEntry win_entries[] = {
  {"save", save_activated, NULL, NULL, NULL},
  {"saveas", saveas_activated, NULL, NULL, NULL},
  {"close", close_activated, NULL, NULL, NULL},
  {"cut", cut_activated, NULL, NULL, NULL},
  {"copy", copy_activated, NULL, NULL, NULL},
  {"paste", paste_activated, NULL, NULL, NULL},
  {"selectall", selectall_activated, NULL, NULL, NULL}
};

/*外部に公開する関数*/

/*MoonedWindowの生成*/

MoonedWindow *mooned_window_new(MoonedApplication *app)
{
  MoonedWindow *win;

  win = g_object_new(MOONED_TYPE_WINDOW, "application", app, NULL);
  win->text_buffer = gtk_text_view_get_buffer(win->text_view);
  win->changed = FALSE;
  win->file = NULL;
  g_action_map_add_action_entries(G_ACTION_MAP(win), win_entries, G_N_ELEMENTS(win_entries), win);
  g_signal_connect(win->text_buffer, "changed", G_CALLBACK(changed_activated), win);
  g_signal_connect(win, "delete-event", G_CALLBACK(delete_event_activated), win);
  return win;
}

/* ファイルをウィンドウに読み込む*/
/*バッファの古い中身は消去される。ただｓ、ファイルからの読み込みがエラーの場合はそのままになる*/
void mooned_window_read(MoonedWindow *win, GFile *file) {
  char *contents;
  gsize length;
  char *filename;

  if (g_file_load_contents(file, NULL, &contents, &length, NULL, NULL)) {
    gtk_text_buffer_set_text(win->text_buffer, contents, length); /*このときchangedシグナルが発生する＝＞win->changed = TRUE*/
    g_free(contents);
    win->file = g_file_dup(file);
    filename = g_file_get_basename(win->file);
    gtk_window_set_title(GTK_WINDOW(win), filename);
    win->changed = FALSE;
    g_free(filename);
  }
}

/* ファイルを読み込んでウィンドウを生成 */
/* ファイルが読み込めなかったら空のウィンドウを返す */
MoonedWindow *mooned_window_open(MoonedApplication *app, GFile *file) {
  MoonedWindow *win;

  win = mooned_window_new(app);
  mooned_window_read(win, file);
  return win;
}

/* 単にポインタを返すだけで、GFileのレファレンス・カウントは変化しない */
/* したがって呼び出し側でGfileが必要なくなっても、g_object_unref()してはいけない */
GFile *mooned_window_get_file(MoonedWindow *win) {
  return win->file;
}

/*ここでいう「空のウィンドウ」とは、1テキストバッファがからである 2対応ファイルがない　ということである*/
/*空のウィンドウの有無により、TRUE、FALSEを返す*/
gboolean mooned_window_is_empty(MoonedWindow *win) {
  GtkTextIter start_iter;
  GtkTextIter end_iter;

  if (win->file)
    return FALSE;
  gtk_text_buffer_get_bounds(win->text_buffer, &start_iter, &end_iter);
  return gtk_text_iter_equal(&start_iter, &end_iter);
}

