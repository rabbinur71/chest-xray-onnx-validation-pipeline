#ifndef XRAY_ENGINE_H
#define XRAY_ENGINE_H

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
|--------------------------------------------------------------------------
| Public Engine API
|--------------------------------------------------------------------------
|
| This is the stable interface that future CLI, GTK3 UI, tests, and export
| modules will use to invoke the offline X-ray analysis engine.
|
| Implementation will be expanded incrementally through Phase 2.
|--------------------------------------------------------------------------
*/

XrayStatus xray_engine_initialize(void);

XrayStatus xray_engine_shutdown(void);

XrayStatus xray_engine_analyze_image(
    const char *image_path,
    XrayAnalysisResult *result
);

#ifdef __cplusplus
}
#endif

#endif /* XRAY_ENGINE_H */
