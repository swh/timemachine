#include <gtk/gtk.h>

void
on_togglebutton1_clicked               (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_window_delete_event                 (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);
