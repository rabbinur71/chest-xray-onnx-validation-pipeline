#ifndef XRAY_LOGGER_H
#define XRAY_LOGGER_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    XRAY_LOG_LEVEL_DEBUG = 0,
    XRAY_LOG_LEVEL_INFO,
    XRAY_LOG_LEVEL_WARN,
    XRAY_LOG_LEVEL_ERROR
} XrayLogLevel;

void xray_log(XrayLogLevel level, const char *module, const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif /* XRAY_LOGGER_H */
