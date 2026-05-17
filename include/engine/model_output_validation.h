#ifndef MODEL_OUTPUT_VALIDATION_H
#define MODEL_OUTPUT_VALIDATION_H

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

XrayStatus model_output_validation_write_probabilities_binary(
    const float probabilities[XRAY_MODEL_OUTPUT_COUNT],
    const char *output_path
);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_OUTPUT_VALIDATION_H */
