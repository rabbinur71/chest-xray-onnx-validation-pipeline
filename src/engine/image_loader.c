#include "engine/image_loader.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <stdlib.h>
#include <string.h>

#include "utils/xray_logger.h"

static bool image_loader_is_supported_channel_count(int channels)
{
    return (channels == 1) || (channels == 3) || (channels == 4);
}

static uint8_t image_loader_rgb_to_luma(uint8_t red, uint8_t green, uint8_t blue)
{
    const float r = (float)red;
    const float g = (float)green;
    const float b = (float)blue;

    const float luma = (0.299F * r) + (0.587F * g) + (0.114F * b);

    if (luma <= 0.0F)
    {
        return 0U;
    }

    if (luma >= 255.0F)
    {
        return 255U;
    }

    return (uint8_t)(luma + 0.5F);
}

XrayStatus image_loader_load_grayscale(
    const char *image_path,
    XrayImage *image
)
{
    if ((image_path == NULL) || (image == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    image->pixels = NULL;
    image->width = 0U;
    image->height = 0U;
    image->channels = 0U;

    GError *error = NULL;

    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(image_path, &error);

    if (pixbuf == NULL)
    {
        if (error != NULL)
        {
            xray_log(
                XRAY_LOG_LEVEL_ERROR,
                "IMAGE_LOADER",
                "Failed to load image '%s': %s",
                image_path,
                error->message
            );

            g_error_free(error);
        }
        else
        {
            xray_log(
                XRAY_LOG_LEVEL_ERROR,
                "IMAGE_LOADER",
                "Failed to load image '%s': unknown error",
                image_path
            );
        }

        return XRAY_STATUS_IMAGE_LOAD_FAILED;
    }

    const int width = gdk_pixbuf_get_width(pixbuf);
    const int height = gdk_pixbuf_get_height(pixbuf);
    const int channels = gdk_pixbuf_get_n_channels(pixbuf);
    const int rowstride = gdk_pixbuf_get_rowstride(pixbuf);
    const gboolean has_alpha = gdk_pixbuf_get_has_alpha(pixbuf);

    if ((width <= 0) || (height <= 0))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "IMAGE_LOADER",
            "Invalid image dimensions: width=%d height=%d",
            width,
            height
        );

        g_object_unref(pixbuf);
        return XRAY_STATUS_IMAGE_LOAD_FAILED;
    }

    if (((size_t)width < XRAY_IMAGE_MIN_WIDTH) ||
        ((size_t)height < XRAY_IMAGE_MIN_HEIGHT))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "IMAGE_LOADER",
            "Image dimensions below minimum threshold: width=%d height=%d minimum=%ux%u",
            width,
            height,
            XRAY_IMAGE_MIN_WIDTH,
            XRAY_IMAGE_MIN_HEIGHT
        );

        g_object_unref(pixbuf);
        return XRAY_STATUS_IMAGE_LOAD_FAILED;
    }

    if (!image_loader_is_supported_channel_count(channels))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "IMAGE_LOADER",
            "Unsupported image properties: width=%d height=%d channels=%d alpha=%d",
            width,
            height,
            channels,
            has_alpha ? 1 : 0
        );

        g_object_unref(pixbuf);
        return XRAY_STATUS_IMAGE_LOAD_FAILED;
    }

    const size_t pixel_count = (size_t)width * (size_t)height;

    uint8_t *grayscale_pixels = calloc(pixel_count, sizeof(uint8_t));

    if (grayscale_pixels == NULL)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "IMAGE_LOADER",
            "Failed to allocate grayscale buffer: pixels=%zu",
            pixel_count
        );

        g_object_unref(pixbuf);
        return XRAY_STATUS_FAILURE;
    }

    const uint8_t *source_pixels = (const uint8_t *)gdk_pixbuf_get_pixels(pixbuf);

    for (int y = 0; y < height; ++y)
    {
        const uint8_t *row = source_pixels + ((size_t)y * (size_t)rowstride);

        for (int x = 0; x < width; ++x)
        {
            const uint8_t *pixel = row + ((size_t)x * (size_t)channels);

            uint8_t value = 0U;

            if (channels == 1)
            {
                value = pixel[0];
            }
            else
            {
                value = image_loader_rgb_to_luma(pixel[0], pixel[1], pixel[2]);
            }

            grayscale_pixels[((size_t)y * (size_t)width) + (size_t)x] = value;
        }
    }

    image->pixels = grayscale_pixels;
    image->width = (size_t)width;
    image->height = (size_t)height;
    image->channels = 1U;

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "IMAGE_LOADER",
        "Loaded grayscale image: path=%s width=%zu height=%zu channels=%zu",
        image_path,
        image->width,
        image->height,
        image->channels
    );

    g_object_unref(pixbuf);

    return XRAY_STATUS_SUCCESS;
}

void image_loader_free(XrayImage *image)
{
    if (image == NULL)
    {
        return;
    }

    free(image->pixels);
    image->pixels = NULL;
    image->width = 0U;
    image->height = 0U;
    image->channels = 0U;
}
