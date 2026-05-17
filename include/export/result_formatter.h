#ifndef RESULT_FORMATTER_H
#define RESULT_FORMATTER_H

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

XrayStatus result_formatter_write_json(
    const XrayAnalysisResult *result,
    const char *output_path
);

XrayStatus result_formatter_print_console(
    const XrayAnalysisResult *result
);

#ifdef __cplusplus
}
#endif

#endif /* RESULT_FORMATTER_H */
