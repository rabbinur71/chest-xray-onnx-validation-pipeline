#include "engine/preprocessor_validation.h"

#include <stdio.h>

#include "utils/xray_logger.h"

XrayStatus preprocessor_validation_write_tensor_binary(
    const XrayTensor *tensor,
    const char *output_path
)
{
    if ((tensor == NULL) ||
        (tensor->data == NULL) ||
        (output_path == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(output_path, "wb");

    if (file == NULL)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "PREPROCESS_VALIDATION",
            "Failed to open tensor output file: %s",
            output_path
        );

        return XRAY_STATUS_FAILURE;
    }

    const size_t elements_written = fwrite(
        tensor->data,
        sizeof(float),
        tensor->total_elements,
        file
    );

    fclose(file);

    if (elements_written != tensor->total_elements)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "PREPROCESS_VALIDATION",
            "Tensor binary write incomplete: expected=%zu actual=%zu",
            tensor->total_elements,
            elements_written
        );

        return XRAY_STATUS_FAILURE;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "PREPROCESS_VALIDATION",
        "Tensor binary exported: path=%s elements=%zu",
        output_path,
        tensor->total_elements
    );

    return XRAY_STATUS_SUCCESS;
}
