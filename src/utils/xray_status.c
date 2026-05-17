#include "engine/xray_types.h"

const char *xray_status_to_string(XrayStatus status)
{
    switch (status)
    {
        case XRAY_STATUS_SUCCESS:
            return "SUCCESS";

        case XRAY_STATUS_FAILURE:
            return "FAILURE";

        case XRAY_STATUS_INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";

        case XRAY_STATUS_FILE_NOT_FOUND:
            return "FILE_NOT_FOUND";

        case XRAY_STATUS_IMAGE_LOAD_FAILED:
            return "IMAGE_LOAD_FAILED";

        case XRAY_STATUS_PREPROCESS_FAILED:
            return "PREPROCESS_FAILED";

        case XRAY_STATUS_MODEL_LOAD_FAILED:
            return "MODEL_LOAD_FAILED";

        case XRAY_STATUS_INFERENCE_FAILED:
            return "INFERENCE_FAILED";

        case XRAY_STATUS_EXPORT_FAILED:
            return "EXPORT_FAILED";

        case XRAY_STATUS_DATABASE_FAILED:
            return "DATABASE_FAILED";

        default:
            return "UNKNOWN_STATUS";
    }
}
