#include "db/database.h"

#include <sqlite3.h>
#include <stdio.h>
#include <string.h>

#include "utils/xray_logger.h"

static const char *XRAY_DATABASE_SCHEMA =
    "CREATE TABLE IF NOT EXISTS studies ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    study_type TEXT NOT NULL,"
    "    image_path TEXT NOT NULL,"
    "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "    inference_completed INTEGER NOT NULL,"
    "    warning TEXT"
    ");"

    "CREATE TABLE IF NOT EXISTS findings ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    study_id INTEGER NOT NULL,"
    "    label TEXT NOT NULL,"
    "    probability REAL NOT NULL,"
    "    status TEXT NOT NULL,"
    "    note TEXT NOT NULL,"
    "    doctor_review_required INTEGER NOT NULL,"
    "    FOREIGN KEY(study_id) REFERENCES studies(id)"
    ");"

    "CREATE TABLE IF NOT EXISTS app_logs ("
    "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "    level TEXT NOT NULL,"
    "    component TEXT NOT NULL,"
    "    message TEXT NOT NULL"
    ");";

static sqlite3 *database_get_sqlite_handle(
    XrayDatabase *database
)
{
    return (sqlite3 *)database->handle;
}

XrayStatus database_initialize(
    XrayDatabase *database,
    const char *database_path
)
{
    if ((database == NULL) || (database_path == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    memset(database, 0, sizeof(*database));

    sqlite3 *handle = NULL;

    const int rc = sqlite3_open(database_path, &handle);

    if ((rc != SQLITE_OK) || (handle == NULL))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "DATABASE",
            "SQLite open failed: %s",
            database_path
        );

        if (handle != NULL)
        {
            sqlite3_close(handle);
        }

        return XRAY_STATUS_DATABASE_FAILED;
    }

    database->handle = handle;

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "DATABASE",
        "SQLite database initialized: %s",
        database_path
    );

    return XRAY_STATUS_SUCCESS;
}

XrayStatus database_create_schema(
    XrayDatabase *database
)
{
    if (database == NULL)
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    sqlite3 *handle = database_get_sqlite_handle(database);

    if (handle == NULL)
    {
        return XRAY_STATUS_DATABASE_FAILED;
    }

    char *error_message = NULL;

    const int rc = sqlite3_exec(
        handle,
        XRAY_DATABASE_SCHEMA,
        NULL,
        NULL,
        &error_message
    );

    if (rc != SQLITE_OK)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "DATABASE",
            "Schema creation failed: %s",
            (error_message != NULL) ? error_message : "unknown"
        );

        sqlite3_free(error_message);

        return XRAY_STATUS_DATABASE_FAILED;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "DATABASE",
        "Database schema verified"
    );

    return XRAY_STATUS_SUCCESS;
}

XrayStatus database_store_analysis_result(
    XrayDatabase *database,
    const XrayAnalysisResult *result
)
{
    if ((database == NULL) || (result == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    sqlite3 *handle = database_get_sqlite_handle(database);

    if (handle == NULL)
    {
        return XRAY_STATUS_DATABASE_FAILED;
    }

    static const char *INSERT_STUDY_SQL =
        "INSERT INTO studies "
        "(study_type, image_path, inference_completed, warning) "
        "VALUES (?, ?, ?, ?);";

    sqlite3_stmt *study_statement = NULL;

    int rc = sqlite3_prepare_v2(
        handle,
        INSERT_STUDY_SQL,
        -1,
        &study_statement,
        NULL
    );

    if (rc != SQLITE_OK)
    {
        return XRAY_STATUS_DATABASE_FAILED;
    }

    sqlite3_bind_text(
        study_statement,
        1,
        result->study_type,
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        study_statement,
        2,
        result->image_path,
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        study_statement,
        3,
        result->inference_completed ? 1 : 0
    );

    sqlite3_bind_text(
        study_statement,
        4,
        result->warning,
        -1,
        SQLITE_TRANSIENT
    );

    rc = sqlite3_step(study_statement);

    sqlite3_finalize(study_statement);

    if (rc != SQLITE_DONE)
    {
        return XRAY_STATUS_DATABASE_FAILED;
    }

    const sqlite3_int64 study_id = sqlite3_last_insert_rowid(handle);

    static const char *INSERT_FINDING_SQL =
        "INSERT INTO findings "
        "(study_id, label, probability, status, note, doctor_review_required) "
        "VALUES (?, ?, ?, ?, ?, ?);";

    for (size_t i = 0U; i < result->finding_count; ++i)
    {
        const FindingResult *finding = &result->findings[i];

        sqlite3_stmt *finding_statement = NULL;

        rc = sqlite3_prepare_v2(
            handle,
            INSERT_FINDING_SQL,
            -1,
            &finding_statement,
            NULL
        );

        if (rc != SQLITE_OK)
        {
            return XRAY_STATUS_DATABASE_FAILED;
        }

        sqlite3_bind_int64(
            finding_statement,
            1,
            study_id
        );

        sqlite3_bind_text(
            finding_statement,
            2,
            finding->label,
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_double(
            finding_statement,
            3,
            finding->probability
        );

        sqlite3_bind_text(
            finding_statement,
            4,
            finding->status,
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_text(
            finding_statement,
            5,
            finding->note,
            -1,
            SQLITE_TRANSIENT
        );

        sqlite3_bind_int(
            finding_statement,
            6,
            finding->doctor_review_required ? 1 : 0
        );

        rc = sqlite3_step(finding_statement);

        sqlite3_finalize(finding_statement);

        if (rc != SQLITE_DONE)
        {
            return XRAY_STATUS_DATABASE_FAILED;
        }
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "DATABASE",
        "Analysis result persisted: findings=%zu",
        result->finding_count
    );

    return XRAY_STATUS_SUCCESS;
}

void database_close(
    XrayDatabase *database
)
{
    if (database == NULL)
    {
        return;
    }

    sqlite3 *handle = database_get_sqlite_handle(database);

    if (handle != NULL)
    {
        sqlite3_close(handle);
        database->handle = NULL;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "DATABASE",
        "Database connection closed"
    );
}
