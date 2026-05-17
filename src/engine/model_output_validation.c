#include "engine/model_output_validation.h"

#include <stdio.h>

#include "utils/xray_logger.h"

XrayStatus model_output_validation_write_probabilities_binary(
    const float probabilities[XRAY_MODEL_OUTPUT_COUNT],
    const char *output_path
)
{
    if ((probabilities == NULL) || (output_path == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    FILE *file = fopen(output_path, "wb");

    if (file == NULL)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "MODEL_OUTPUT_VALIDATION",
            "Failed to open probability output file: %s",
            output_path
        );

        return XRAY_STATUS_FAILURE;
    }

    const size_t elements_written = fwrite(
        probabilities,
        sizeof(float),
        XRAY_MODEL_OUTPUT_COUNT,
        file
    );

    fclose(file);

    if (elements_written != XRAY_MODEL_OUTPUT_COUNT)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "MODEL_OUTPUT_VALIDATION",
            "Probability binary write incomplete: expected=%u actual=%zu",
            XRAY_MODEL_OUTPUT_COUNT,
            elements_written
        );

        return XRAY_STATUS_FAILURE;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "MODEL_OUTPUT_VALIDATION",
        "Probability binary exported: path=%s elements=%u",
        output_path,
        XRAY_MODEL_OUTPUT_COUNT
    );

    return XRAY_STATUS_SUCCESS;
}
