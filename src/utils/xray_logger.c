#include "utils/xray_logger.h"

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

static const char *xray_log_level_to_string(XrayLogLevel level)
{
    switch (level)
    {
        case XRAY_LOG_LEVEL_DEBUG:
            return "DEBUG";

        case XRAY_LOG_LEVEL_INFO:
            return "INFO";

        case XRAY_LOG_LEVEL_WARN:
            return "WARN";

        case XRAY_LOG_LEVEL_ERROR:
            return "ERROR";

        default:
            return "UNKNOWN";
    }
}

void xray_log(XrayLogLevel level, const char *module, const char *format, ...)
{
    time_t now = time(NULL);

    struct tm *time_info = localtime(&now);

    if (time_info == NULL)
    {
        fprintf(stderr, "[LOGGER_FAILURE] Failed to obtain local time\n");
        return;
    }

    char timestamp[32];

    if (strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", time_info) == 0U)
    {
        fprintf(stderr, "[LOGGER_FAILURE] Failed to format timestamp\n");
        return;
    }

    fprintf(
        stdout,
        "[%s] [%s] [%s] ",
        timestamp,
        xray_log_level_to_string(level),
        module
    );

    va_list args;

    va_start(args, format);
    vfprintf(stdout, format, args);
    va_end(args);

    fprintf(stdout, "\n");
}
