#ifndef XRAY_TYPES_H
#define XRAY_TYPES_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
|--------------------------------------------------------------------------
| Global Constants
|--------------------------------------------------------------------------
*/

#define XRAY_MAX_FINDINGS            32U
#define XRAY_MAX_LABEL_LENGTH        64U
#define XRAY_MAX_STATUS_LENGTH       64U
#define XRAY_MAX_NOTE_LENGTH         256U
#define XRAY_MAX_PATH_LENGTH         512U
#define XRAY_MAX_WARNING_LENGTH      512U
#define XRAY_MODEL_OUTPUT_COUNT      18U

/*
|--------------------------------------------------------------------------
| Engine Status Codes
|--------------------------------------------------------------------------
*/

typedef enum
{
    XRAY_STATUS_SUCCESS = 0,
    XRAY_STATUS_FAILURE,
    XRAY_STATUS_INVALID_ARGUMENT,
    XRAY_STATUS_FILE_NOT_FOUND,
    XRAY_STATUS_IMAGE_LOAD_FAILED,
    XRAY_STATUS_PREPROCESS_FAILED,
    XRAY_STATUS_MODEL_LOAD_FAILED,
    XRAY_STATUS_INFERENCE_FAILED,
    XRAY_STATUS_EXPORT_FAILED,
    XRAY_STATUS_DATABASE_FAILED
} XrayStatus;

/*
|--------------------------------------------------------------------------
| Finding Result Structure
|--------------------------------------------------------------------------
*/

typedef struct
{
    char label[XRAY_MAX_LABEL_LENGTH];
    float probability;
    char status[XRAY_MAX_STATUS_LENGTH];
    char note[XRAY_MAX_NOTE_LENGTH];
    bool doctor_review_required;
} FindingResult;

/*
|--------------------------------------------------------------------------
| Main Analysis Result Structure
|--------------------------------------------------------------------------
*/

typedef struct
{
    char study_type[64];
    char image_path[XRAY_MAX_PATH_LENGTH];

    FindingResult findings[XRAY_MAX_FINDINGS];
    size_t finding_count;

    char warning[XRAY_MAX_WARNING_LENGTH];

    bool inference_completed;
} XrayAnalysisResult;

/*
|--------------------------------------------------------------------------
| Image Tensor Structure
|--------------------------------------------------------------------------
*/

typedef struct
{
    float *data;

    size_t batch;
    size_t channels;
    size_t height;
    size_t width;

    size_t total_elements;
} XrayTensor;

/*
|--------------------------------------------------------------------------
| Utility Helpers
|--------------------------------------------------------------------------
*/

const char *xray_status_to_string(XrayStatus status);

#ifdef __cplusplus
}
#endif

#endif /* XRAY_TYPES_H */
