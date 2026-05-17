#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include "engine/xray_types.h"

#define XRAY_IMAGE_MIN_WIDTH   128U
#define XRAY_IMAGE_MIN_HEIGHT  128U

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint8_t *pixels;
    size_t width;
    size_t height;
    size_t channels;
} XrayImage;

XrayStatus image_loader_load_grayscale(
    const char *image_path,
    XrayImage *image
);

void image_loader_free(XrayImage *image);

#ifdef __cplusplus
}
#endif

#endif /* IMAGE_LOADER_H */
