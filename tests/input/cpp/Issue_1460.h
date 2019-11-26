#define MGT_TYPE_WINDOW (mgt_window_get_type ())

G_DECLARE_FINAL_TYPE (MgtWindow, mgt_window, MGT, WINDOW, GtkApplicationWindow)

MgtWindow *mgt_window_new (MgtApplication *app);
