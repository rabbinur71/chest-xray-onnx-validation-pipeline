#include "engine/preprocessor.h"

#include <math.h>
#include <stdlib.h>

#include "utils/xray_logger.h"

static uint8_t preprocessor_sample_area(
    const XrayImage *image,
    size_t target_x,
    size_t target_y
)
{
    const double scale_x = (double)image->width / (double)XRAY_MODEL_INPUT_WIDTH;
    const double scale_y = (double)image->height / (double)XRAY_MODEL_INPUT_HEIGHT;

    const double source_x_start = (double)target_x * scale_x;
    const double source_x_end = (double)(target_x + 1U) * scale_x;
    const double source_y_start = (double)target_y * scale_y;
    const double source_y_end = (double)(target_y + 1U) * scale_y;

    size_t x_start = (size_t)floor(source_x_start);
    size_t x_end = (size_t)ceil(source_x_end);
    size_t y_start = (size_t)floor(source_y_start);
    size_t y_end = (size_t)ceil(source_y_end);

    if (x_end > image->width)
    {
        x_end = image->width;
    }

    if (y_end > image->height)
    {
        y_end = image->height;
    }

    if (x_start >= image->width)
    {
        x_start = image->width - 1U;
    }

    if (y_start >= image->height)
    {
        y_start = image->height - 1U;
    }

    double weighted_sum = 0.0;
    double total_weight = 0.0;

    for (size_t source_y = y_start; source_y < y_end; ++source_y)
    {
        const double pixel_y_start = (double)source_y;
        const double pixel_y_end = (double)(source_y + 1U);

        const double overlap_y_start =
            (source_y_start > pixel_y_start) ? source_y_start : pixel_y_start;
        const double overlap_y_end =
            (source_y_end < pixel_y_end) ? source_y_end : pixel_y_end;

        const double weight_y = overlap_y_end - overlap_y_start;

        if (weight_y <= 0.0)
        {
            continue;
        }

        for (size_t source_x = x_start; source_x < x_end; ++source_x)
        {
            const double pixel_x_start = (double)source_x;
            const double pixel_x_end = (double)(source_x + 1U);

            const double overlap_x_start =
                (source_x_start > pixel_x_start) ? source_x_start : pixel_x_start;
            const double overlap_x_end =
                (source_x_end < pixel_x_end) ? source_x_end : pixel_x_end;

            const double weight_x = overlap_x_end - overlap_x_start;

            if (weight_x <= 0.0)
            {
                continue;
            }

            const double weight = weight_x * weight_y;
            const uint8_t pixel = image->pixels[(source_y * image->width) + source_x];

            weighted_sum += (double)pixel * weight;
            total_weight += weight;
        }
    }

    if (total_weight <= 0.0)
    {
        return 0U;
    }

    const double average = weighted_sum / total_weight;

    if (average <= 0.0)
    {
        return 0U;
    }

    if (average >= 255.0)
    {
        return 255U;
    }

    return (uint8_t)(average + 0.5);
}

static float preprocessor_normalize_uint8(uint8_t pixel)
{
    return (((float)pixel / 255.0F) * 2048.0F) - 1024.0F;
}

XrayStatus preprocessor_create_tensor(
    const XrayImage *image,
    XrayTensor *tensor
)
{
    if ((image == NULL) || (tensor == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    if ((image->pixels == NULL) ||
        (image->width == 0U) ||
        (image->height == 0U) ||
        (image->channels != 1U))
    {
        return XRAY_STATUS_PREPROCESS_FAILED;
    }

    tensor->data = NULL;
    tensor->batch = 1U;
    tensor->channels = XRAY_MODEL_INPUT_CHANNELS;
    tensor->height = XRAY_MODEL_INPUT_HEIGHT;
    tensor->width = XRAY_MODEL_INPUT_WIDTH;
    tensor->total_elements =
        tensor->batch *
        tensor->channels *
        tensor->height *
        tensor->width;

    tensor->data = calloc(tensor->total_elements, sizeof(float));

    if (tensor->data == NULL)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "PREPROCESSOR",
            "Failed to allocate tensor buffer: elements=%zu",
            tensor->total_elements
        );

        return XRAY_STATUS_PREPROCESS_FAILED;
    }

    float tensor_min = 1024.0F;
    float tensor_max = -1024.0F;
    double tensor_sum = 0.0;

    for (size_t y = 0U; y < XRAY_MODEL_INPUT_HEIGHT; ++y)
    {
        for (size_t x = 0U; x < XRAY_MODEL_INPUT_WIDTH; ++x)
        {
            const uint8_t pixel = preprocessor_sample_area(image, x, y);
            const float normalized = preprocessor_normalize_uint8(pixel);

            const size_t index = y * XRAY_MODEL_INPUT_WIDTH + x;

            tensor->data[index] = normalized;

            if (normalized < tensor_min)
            {
                tensor_min = normalized;
            }

            if (normalized > tensor_max)
            {
                tensor_max = normalized;
            }

            tensor_sum += (double)normalized;
        }
    }

    const double tensor_mean =
        tensor_sum / (double)tensor->total_elements;

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "PREPROCESSOR",
        "Tensor created: shape=[%zu,%zu,%zu,%zu] elements=%zu",
        tensor->batch,
        tensor->channels,
        tensor->height,
        tensor->width,
        tensor->total_elements
    );

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "PREPROCESSOR",
        "Tensor statistics: min=%.4f max=%.4f mean=%.4f",
        (double)tensor_min,
        (double)tensor_max,
        tensor_mean
    );

    return XRAY_STATUS_SUCCESS;
}

void preprocessor_free_tensor(XrayTensor *tensor)
{
    if (tensor == NULL)
    {
        return;
    }

    free(tensor->data);
    tensor->data = NULL;
    tensor->batch = 0U;
    tensor->channels = 0U;
    tensor->height = 0U;
    tensor->width = 0U;
    tensor->total_elements = 0U;
}
