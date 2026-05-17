#ifndef MODEL_RUNNER_H
#define MODEL_RUNNER_H

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ModelRunner ModelRunner;

XrayStatus model_runner_create(
    const char *model_path,
    ModelRunner **runner
);

XrayStatus model_runner_run(
    ModelRunner *runner,
    const XrayTensor *input_tensor,
    float output_probabilities[XRAY_MODEL_OUTPUT_COUNT]
);

void model_runner_destroy(ModelRunner *runner);

#ifdef __cplusplus
}
#endif

#endif /* MODEL_RUNNER_H */
