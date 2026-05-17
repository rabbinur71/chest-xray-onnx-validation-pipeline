#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <gtk/gtk.h>

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    GtkWidget *window;
    GtkWidget *image_preview;
    GtkWidget *status_label;
    GtkWidget *analyze_button;
    GtkWidget *export_button;

    GtkListStore *findings_store;
    GtkWidget *report_text_view;

    GtkListStore *history_store;

    gboolean analysis_running;
    XrayStatus last_analysis_status;

    XrayAnalysisResult last_result;

    char selected_image_path[512];
    char last_report_path[512];
} MainWindowState;

GtkWidget *main_window_create(GtkApplication *application);

#ifdef __cplusplus
}
#endif

#endif /* MAIN_WINDOW_H */
