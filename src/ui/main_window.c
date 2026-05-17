#include "ui/main_window.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "engine/xray_engine.h"
#include "engine/xray_types.h"
#include "engine/xray_runtime_paths.h"

#include <sqlite3.h>

static void main_window_set_status(
    MainWindowState *state,
    const char *message
)
{
    if ((state == NULL) || (state->status_label == NULL) || (message == NULL))
    {
        return;
    }

    gtk_label_set_text(GTK_LABEL(state->status_label), message);
}

static void main_window_show_error(
    MainWindowState *state,
    const char *message
)
{
    if ((state == NULL) || (state->window == NULL) || (message == NULL))
    {
        return;
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        "%s",
        message
    );

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

static int main_window_file_is_readable(const char *path)
{
    if (path == NULL)
    {
        return 0;
    }

    return access(path, R_OK) == 0;
}

static void main_window_load_preview_image(
    MainWindowState *state,
    const char *image_path
)
{
    if ((state == NULL) || (image_path == NULL))
    {
        return;
    }

    GError *error = NULL;

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
        image_path,
        720,
        640,
        TRUE,
        &error
    );

    if (pixbuf == NULL)
    {
        const char *error_message = (error != NULL) ? error->message : "Unknown image loading error";

        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(state->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Failed to load image preview:\n%s",
            error_message
        );

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        if (error != NULL)
        {
            g_error_free(error);
        }

        main_window_set_status(state, "Image preview failed");
        return;
    }

    gtk_image_set_from_pixbuf(GTK_IMAGE(state->image_preview), pixbuf);
    g_object_unref(pixbuf);

    main_window_set_status(state, image_path);
    gtk_widget_set_sensitive(state->analyze_button, TRUE);
}

static void main_window_update_findings_table(MainWindowState *state)
{
    if ((state == NULL) || (state->findings_store == NULL))
    {
        return;
    }

    gtk_list_store_clear(state->findings_store);

    for (size_t i = 0U; i < state->last_result.finding_count; ++i)
    {
        const FindingResult *finding = &state->last_result.findings[i];

        GtkTreeIter iter;

        gtk_list_store_append(state->findings_store, &iter);
        gtk_list_store_set(
            state->findings_store,
            &iter,
            0, finding->label,
            1, (double)finding->probability,
            2, finding->status,
            3, finding->doctor_review_required ? "Yes" : "No",
            -1
        );
    }
}

static void main_window_build_report_path(MainWindowState *state)
{
    if ((state == NULL) || (state->selected_image_path[0] == '\0'))
    {
        return;
    }

    const char *filename = strrchr(state->selected_image_path, '/');

    if (filename == NULL)
    {
        filename = state->selected_image_path;
    }
    else
    {
        filename += 1;
    }

    char image_stem[128] = {0};
    const char *extension = strrchr(filename, '.');

    size_t stem_length = (extension == NULL)
        ? strlen(filename)
        : (size_t)(extension - filename);

    if (stem_length >= sizeof(image_stem))
    {
        stem_length = sizeof(image_stem) - 1U;
    }

    memcpy(image_stem, filename, stem_length);
    image_stem[stem_length] = '\0';

    (void)snprintf(
        state->last_report_path,
        sizeof(state->last_report_path),
        XRAY_REPORT_DIRECTORY "/report_%s.json",
        image_stem
    );
}

static void on_export_report_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    (void)button;

    MainWindowState *state = (MainWindowState *)user_data;

    if ((state == NULL) || (state->last_report_path[0] == '\0'))
    {
        return;
    }

    GtkWidget *dialog = gtk_message_dialog_new(
        GTK_WINDOW(state->window),
        GTK_DIALOG_MODAL,
        GTK_MESSAGE_INFO,
        GTK_BUTTONS_CLOSE,
        "Report generated at:\n%s",
        state->last_report_path
    );

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    main_window_set_status(state, state->last_report_path);
}

static void main_window_update_report_view(MainWindowState *state)
{
    if ((state == NULL) || (state->report_text_view == NULL))
    {
        return;
    }

    GtkTextBuffer *buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(state->report_text_view)
    );

    char report_text[8192];

    int written = snprintf(
        report_text,
        sizeof(report_text),
        "AI-Assisted Chest X-Ray Analysis Report\n"
        "============================================================\n"
        "Study Type: %s\n"
        "Image Path : %s\n"
        "Completed  : %s\n"
        "Warning    : %s\n"
        "Findings   : %llu\n"
        "------------------------------------------------------------\n",
        state->last_result.study_type,
        state->last_result.image_path,
        state->last_result.inference_completed ? "yes" : "no",
        state->last_result.warning,
        (unsigned long long)state->last_result.finding_count
    );

    if (written < 0)
    {
        gtk_text_buffer_set_text(buffer, "Report rendering failed.", -1);
        return;
    }

    size_t offset = (size_t)written;

    if (state->last_result.finding_count == 0U)
    {
        written = snprintf(
            report_text + offset,
            sizeof(report_text) - offset,
            "No primary suspicious findings generated by the rule engine.\n"
        );

        if (written > 0)
        {
            offset += (size_t)written;
        }
    }

    for (size_t i = 0U;
         (i < state->last_result.finding_count) && (offset < sizeof(report_text));
         ++i)
    {
        const FindingResult *finding = &state->last_result.findings[i];

        written = snprintf(
            report_text + offset,
            sizeof(report_text) - offset,
            "%llu. %s | probability=%.6f | status=%s\n"
            "   Note: %s\n",
            (unsigned long long)(i + 1U),
            finding->label,
            (double)finding->probability,
            finding->status,
            finding->note
        );

        if (written < 0)
        {
            gtk_text_buffer_set_text(buffer, "Report rendering failed.", -1);
            return;
        }

        offset += (size_t)written;
    }

    if (offset >= sizeof(report_text))
    {
        report_text[sizeof(report_text) - 1U] = '\0';
    }

    gtk_text_buffer_set_text(buffer, report_text, -1);
}

static void main_window_load_history(MainWindowState *state)
{
    if ((state == NULL) || (state->history_store == NULL))
    {
        return;
    }

    gtk_list_store_clear(state->history_store);

    sqlite3 *database = NULL;

    if (sqlite3_open(XRAY_DATABASE_PATH, &database) != SQLITE_OK)
    {
        main_window_set_status(state, "No analysis history database available");
        if (database != NULL)
        {
            sqlite3_close(database);
        }
        return;
    }

    const char *sql =
        "SELECT "
        "s.id, "
        "s.created_at, "
        "s.image_path, "
        "COUNT(f.id) AS finding_count "
        "FROM studies s "
        "LEFT JOIN findings f ON f.study_id = s.id "
        "GROUP BY s.id "
        "ORDER BY s.id DESC "
        "LIMIT 25;";

    sqlite3_stmt *statement = NULL;

    if (sqlite3_prepare_v2(database, sql, -1, &statement, NULL) != SQLITE_OK)
    {
        sqlite3_close(database);
        main_window_set_status(state, "Failed to load analysis history");
        return;
    }

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const int study_id = sqlite3_column_int(statement, 0);
        const unsigned char *created_at = sqlite3_column_text(statement, 1);
        const unsigned char *image_path = sqlite3_column_text(statement, 2);
        const int finding_count = sqlite3_column_int(statement, 3);

        GtkTreeIter iter;

        gtk_list_store_append(state->history_store, &iter);
        gtk_list_store_set(
            state->history_store,
            &iter,
            0, study_id,
            1, (created_at != NULL) ? (const char *)created_at : "",
            2, (image_path != NULL) ? (const char *)image_path : "",
            3, finding_count,
            -1
        );
    }

    sqlite3_finalize(statement);
    sqlite3_close(database);
}

static void on_open_image_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    (void)button;

    MainWindowState *state = (MainWindowState *)user_data;

    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Open Chest X-Ray Image",
        GTK_WINDOW(state->window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel",
        GTK_RESPONSE_CANCEL,
        "_Open",
        GTK_RESPONSE_ACCEPT,
        NULL
    );

    GtkFileFilter *filter = gtk_file_filter_new();

    gtk_file_filter_set_name(filter, "Image Files (*.jpg, *.jpeg, *.png)");
    gtk_file_filter_add_pattern(filter, "*.jpg");
    gtk_file_filter_add_pattern(filter, "*.jpeg");
    gtk_file_filter_add_pattern(filter, "*.png");
    gtk_file_filter_add_pattern(filter, "*.JPG");
    gtk_file_filter_add_pattern(filter, "*.JPEG");
    gtk_file_filter_add_pattern(filter, "*.PNG");

    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    const gint response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

        if (filename != NULL)
        {
            (void)snprintf(
                state->selected_image_path,
                sizeof(state->selected_image_path),
                "%s",
                filename
            );

            main_window_load_preview_image(state, state->selected_image_path);

            g_free(filename);
        }
    }

    gtk_widget_destroy(dialog);
}

static gboolean main_window_analysis_completed_on_ui_thread(gpointer user_data)
{
    MainWindowState *state = (MainWindowState *)user_data;

    if (state == NULL)
    {
        return G_SOURCE_REMOVE;
    }

    state->analysis_running = FALSE;

    if (state->last_analysis_status != XRAY_STATUS_SUCCESS)
    {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(state->window),
            GTK_DIALOG_MODAL,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_CLOSE,
            "Analysis failed: %s",
            xray_status_to_string(state->last_analysis_status)
        );

        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);

        main_window_set_status(state, "Analysis failed");
        gtk_widget_set_sensitive(state->analyze_button, TRUE);
        gtk_widget_set_sensitive(state->export_button, FALSE);

        return G_SOURCE_REMOVE;
    }

    main_window_build_report_path(state);
    main_window_update_findings_table(state);
    main_window_update_report_view(state);
    main_window_load_history(state);

    main_window_set_status(state, "Analysis completed successfully");
    gtk_widget_set_sensitive(state->analyze_button, TRUE);
    gtk_widget_set_sensitive(state->export_button, TRUE);

    return G_SOURCE_REMOVE;
}

static gpointer main_window_analysis_worker(gpointer user_data)
{
    MainWindowState *state = (MainWindowState *)user_data;

    if (state == NULL)
    {
        return NULL;
    }

    XrayStatus status = xray_engine_initialize();

    if (status == XRAY_STATUS_SUCCESS)
    {
        status = xray_engine_analyze_image(
            state->selected_image_path,
            &state->last_result
        );

        (void)xray_engine_shutdown();
    }

    state->last_analysis_status = status;

    g_idle_add(
        main_window_analysis_completed_on_ui_thread,
        state
    );

    return NULL;
}

static void on_analyze_clicked(
    GtkButton *button,
    gpointer user_data
)
{
    (void)button;

    MainWindowState *state = (MainWindowState *)user_data;

    if (state == NULL)
    {
        return;
    }

    if (state->analysis_running)
    {
        return;
    }

    if (state->selected_image_path[0] == '\0')
    {
        main_window_show_error(state, "No image has been selected.");
        main_window_set_status(state, "No image selected");
        return;
    }

    if (!main_window_file_is_readable(XRAY_MODEL_PATH))
    {
        main_window_show_error(
            state,
            "Required model file is missing or unreadable:\n" XRAY_MODEL_PATH
        );
        main_window_set_status(state, "Model file missing");
        return;
    }

    if (!main_window_file_is_readable(state->selected_image_path))
    {
        main_window_show_error(
            state,
            "Selected image file is missing or unreadable."
        );
        main_window_set_status(state, "Selected image unreadable");
        return;
    }

    state->analysis_running = TRUE;
    state->last_analysis_status = XRAY_STATUS_FAILURE;

    gtk_widget_set_sensitive(state->analyze_button, FALSE);
    gtk_widget_set_sensitive(state->export_button, FALSE);

    main_window_set_status(state, "Running analysis...");

    GThread *worker = g_thread_new(
        "xray-analysis-worker",
        main_window_analysis_worker,
        state
    );

    if (worker == NULL)
    {
        state->analysis_running = FALSE;
        main_window_show_error(state, "Failed to start analysis worker thread.");
        main_window_set_status(state, "Analysis worker failed");
        gtk_widget_set_sensitive(state->analyze_button, TRUE);
        return;
    }

    g_thread_unref(worker);
}

GtkWidget *main_window_create(GtkApplication *application)
{
    MainWindowState *state = g_malloc0(sizeof(MainWindowState));

    GtkWidget *window = gtk_application_window_new(application);

    state->window = window;

    gtk_window_set_title(GTK_WINDOW(window), "XRayDoctorAssist");
    gtk_window_set_default_size(GTK_WINDOW(window), 1280, 820);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);

    GtkWidget *root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), root);

    GtkWidget *toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_set_margin_top(toolbar, 10);
    gtk_widget_set_margin_bottom(toolbar, 10);
    gtk_widget_set_margin_start(toolbar, 10);
    gtk_widget_set_margin_end(toolbar, 10);

    GtkWidget *open_button = gtk_button_new_with_label("Open Image");
    GtkWidget *analyze_button = gtk_button_new_with_label("Analyze");
    GtkWidget *export_button = gtk_button_new_with_label("Export Report");

    state->analyze_button = analyze_button;
    state->export_button = export_button;

    gtk_widget_set_sensitive(analyze_button, FALSE);
    gtk_widget_set_sensitive(export_button, FALSE);

    gtk_box_pack_start(GTK_BOX(toolbar), open_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), analyze_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(toolbar), export_button, FALSE, FALSE, 0);

    GtkWidget *toolbar_spacer = gtk_label_new("");
    gtk_widget_set_hexpand(toolbar_spacer, TRUE);
    gtk_box_pack_start(GTK_BOX(toolbar), toolbar_spacer, TRUE, TRUE, 0);

    GtkWidget *developer_label = gtk_label_new("Developed by Raaz");
    gtk_widget_set_halign(developer_label, GTK_ALIGN_END);
    gtk_box_pack_end(GTK_BOX(toolbar), developer_label, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(root), toolbar, FALSE, FALSE, 0);

    GtkWidget *main_paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_set_vexpand(main_paned, TRUE);
    gtk_widget_set_hexpand(main_paned, TRUE);
    gtk_box_pack_start(GTK_BOX(root), main_paned, TRUE, TRUE, 0);

    GtkWidget *image_frame = gtk_frame_new("Image Preview");
    gtk_widget_set_margin_start(image_frame, 10);
    gtk_widget_set_margin_bottom(image_frame, 10);

    GtkWidget *image_preview = gtk_image_new();
    state->image_preview = image_preview;

    gtk_widget_set_hexpand(image_preview, TRUE);
    gtk_widget_set_vexpand(image_preview, TRUE);
    gtk_container_add(GTK_CONTAINER(image_frame), image_preview);

    gtk_paned_pack1(GTK_PANED(main_paned), image_frame, TRUE, FALSE);

    GtkWidget *right_panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_margin_start(right_panel, 10);
    gtk_widget_set_margin_end(right_panel, 10);
    gtk_widget_set_margin_bottom(right_panel, 10);

    GtkWidget *findings_frame = gtk_frame_new("Primary Findings");
    gtk_widget_set_vexpand(findings_frame, TRUE);

    GtkListStore *findings_store = gtk_list_store_new(
        4,
        G_TYPE_STRING,
        G_TYPE_DOUBLE,
        G_TYPE_STRING,
        G_TYPE_STRING
    );

    state->findings_store = findings_store;

    GtkWidget *findings_view = gtk_tree_view_new_with_model(
        GTK_TREE_MODEL(findings_store)
    );

    GtkCellRenderer *text_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(findings_view),
        -1,
        "Finding",
        text_renderer,
        "text",
        0,
        NULL
    );

    GtkCellRenderer *probability_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(findings_view),
        -1,
        "Probability",
        probability_renderer,
        "text",
        1,
        NULL
    );

    GtkCellRenderer *status_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(findings_view),
        -1,
        "Status",
        status_renderer,
        "text",
        2,
        NULL
    );

    GtkCellRenderer *review_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(findings_view),
        -1,
        "Review Required",
        review_renderer,
        "text",
        3,
        NULL
    );

    GtkWidget *findings_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(findings_scroll), findings_view);
    gtk_container_add(GTK_CONTAINER(findings_frame), findings_scroll);

    GtkWidget *report_frame = gtk_frame_new("Report Summary");
    gtk_widget_set_vexpand(report_frame, TRUE);

    GtkWidget *report_text_view = gtk_text_view_new();
    state->report_text_view = report_text_view;

    gtk_text_view_set_editable(GTK_TEXT_VIEW(report_text_view), FALSE);
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(report_text_view), GTK_WRAP_WORD_CHAR);

    GtkTextBuffer *initial_buffer = gtk_text_view_get_buffer(
        GTK_TEXT_VIEW(report_text_view)
    );

    gtk_text_buffer_set_text(
        initial_buffer,
        "Run analysis to generate a structured report.",
        -1
    );

    GtkWidget *report_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(report_scroll), report_text_view);
    gtk_container_add(GTK_CONTAINER(report_frame), report_scroll);

    GtkWidget *history_frame = gtk_frame_new("Recent Studies");
    gtk_widget_set_vexpand(history_frame, TRUE);

    GtkListStore *history_store = gtk_list_store_new(
        4,
        G_TYPE_INT,
        G_TYPE_STRING,
        G_TYPE_STRING,
        G_TYPE_INT
    );

    state->history_store = history_store;

    GtkWidget *history_view = gtk_tree_view_new_with_model(
        GTK_TREE_MODEL(history_store)
    );

    GtkCellRenderer *history_id_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(history_view),
        -1,
        "ID",
        history_id_renderer,
        "text",
        0,
        NULL
    );

    GtkCellRenderer *history_time_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(history_view),
        -1,
        "Created",
        history_time_renderer,
        "text",
        1,
        NULL
    );

    GtkCellRenderer *history_path_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(history_view),
        -1,
        "Image",
        history_path_renderer,
        "text",
        2,
        NULL
    );

    GtkCellRenderer *history_count_renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_insert_column_with_attributes(
        GTK_TREE_VIEW(history_view),
        -1,
        "Findings",
        history_count_renderer,
        "text",
        3,
        NULL
    );

    GtkWidget *history_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(history_scroll), history_view);
    gtk_container_add(GTK_CONTAINER(history_frame), history_scroll);

    gtk_box_pack_start(GTK_BOX(right_panel), findings_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right_panel), report_frame, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right_panel), history_frame, TRUE, TRUE, 0);

    main_window_load_history(state);

    gtk_paned_pack2(GTK_PANED(main_paned), right_panel, TRUE, FALSE);
    gtk_paned_set_position(GTK_PANED(main_paned), 760);

    GtkWidget *status_label = gtk_label_new("Ready");
    state->status_label = status_label;

    gtk_widget_set_halign(status_label, GTK_ALIGN_START);
    gtk_widget_set_margin_start(status_label, 10);
    gtk_widget_set_margin_end(status_label, 10);
    gtk_widget_set_margin_bottom(status_label, 8);

    gtk_box_pack_end(GTK_BOX(root), status_label, FALSE, FALSE, 0);

    g_signal_connect(open_button, "clicked", G_CALLBACK(on_open_image_clicked), state);
    g_signal_connect(analyze_button, "clicked", G_CALLBACK(on_analyze_clicked), state);
    g_signal_connect(export_button, "clicked", G_CALLBACK(on_export_report_clicked), state);

    g_object_set_data_full(
        G_OBJECT(window),
        "main-window-state",
        state,
        g_free
    );

    return window;
}
