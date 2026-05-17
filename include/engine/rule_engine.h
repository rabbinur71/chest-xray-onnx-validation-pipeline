#ifndef RULE_ENGINE_H
#define RULE_ENGINE_H

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    size_t index;
    char label[XRAY_MAX_LABEL_LENGTH];

    float low_threshold;
    float medium_threshold;
    float high_threshold;
} XrayRuleDefinition;

typedef struct
{
    XrayRuleDefinition rules[XRAY_MODEL_OUTPUT_COUNT];
    size_t rule_count;
} XrayRuleSet;

XrayStatus rule_engine_load_default_rules(
    XrayRuleSet *rule_set
);

XrayStatus rule_engine_evaluate(
    const XrayRuleSet *rule_set,
    const float probabilities[XRAY_MODEL_OUTPUT_COUNT],
    XrayAnalysisResult *result
);

#ifdef __cplusplus
}
#endif

#endif /* RULE_ENGINE_H */
