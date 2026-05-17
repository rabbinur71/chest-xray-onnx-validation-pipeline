#ifndef PREPROCESSOR_VALIDATION_H
#define PREPROCESSOR_VALIDATION_H

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

XrayStatus preprocessor_validation_write_tensor_binary(
    const XrayTensor *tensor,
    const char *output_path
);

#ifdef __cplusplus
}
#endif

#endif /* PREPROCESSOR_VALIDATION_H */
