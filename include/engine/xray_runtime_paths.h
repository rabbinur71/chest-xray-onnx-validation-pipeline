#ifndef XRAY_RUNTIME_PATHS_H
#define XRAY_RUNTIME_PATHS_H

#ifdef __cplusplus
extern "C" {
#endif

/*
|--------------------------------------------------------------------------
| Runtime Paths
|--------------------------------------------------------------------------
|
| Linux development:
|   - relative project-local paths
|
| Windows installer:
|   - application binaries/models are under Program Files
|   - writable data/reports are under ProgramData
|--------------------------------------------------------------------------
*/

#ifdef _WIN32

#define XRAY_MODEL_PATH \
    "models\\chest_xray_model.onnx"

#define XRAY_DATABASE_PATH \
    "C:\\ProgramData\\XRayDoctorAssist\\data\\app.sqlite"

#define XRAY_REPORT_DIRECTORY \
    "C:\\ProgramData\\XRayDoctorAssist\\reports\\final_reports"

#define XRAY_PREPROCESSING_VALIDATION_DIRECTORY \
    "C:\\ProgramData\\XRayDoctorAssist\\reports\\preprocessing_validation"

#define XRAY_ONNX_VALIDATION_DIRECTORY \
    "C:\\ProgramData\\XRayDoctorAssist\\reports\\onnx_c_runtime_validation"

#else

#define XRAY_MODEL_PATH \
    "models/chest_xray_model.onnx"

#define XRAY_DATABASE_PATH \
    "data/app.sqlite"

#define XRAY_REPORT_DIRECTORY \
    "reports/final_reports"

#define XRAY_PREPROCESSING_VALIDATION_DIRECTORY \
    "reports/preprocessing_validation"

#define XRAY_ONNX_VALIDATION_DIRECTORY \
    "reports/onnx_c_runtime_validation"

#endif

#ifdef __cplusplus
}
#endif

#endif /* XRAY_RUNTIME_PATHS_H */
