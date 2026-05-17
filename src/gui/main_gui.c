#include <gtk/gtk.h>

#include "ui/main_window.h"

static void on_activate(GtkApplication *application, gpointer user_data)
{
    (void)user_data;

    GtkWidget *window = main_window_create(application);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv)
{
    GtkApplication *application = gtk_application_new(
        "com.xraydoctorassist.desktop",
        G_APPLICATION_DEFAULT_FLAGS
    );

    g_signal_connect(application, "activate", G_CALLBACK(on_activate), NULL);

    const int status = g_application_run(
        G_APPLICATION(application),
        argc,
        argv
    );

    g_object_unref(application);

    return status;
}
