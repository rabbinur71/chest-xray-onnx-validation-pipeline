#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include "engine/xray_types.h"
#include "engine/image_loader.h"

#ifdef __cplusplus
extern "C" {
#endif

#define XRAY_MODEL_INPUT_WIDTH   224U
#define XRAY_MODEL_INPUT_HEIGHT  224U
#define XRAY_MODEL_INPUT_CHANNELS 1U

XrayStatus preprocessor_create_tensor(
    const XrayImage *image,
    XrayTensor *tensor
);

void preprocessor_free_tensor(XrayTensor *tensor);

#ifdef __cplusplus
}
#endif

#endif /* PREPROCESSOR_H */
