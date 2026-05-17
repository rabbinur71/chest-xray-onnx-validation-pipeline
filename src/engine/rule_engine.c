#include "engine/rule_engine.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "utils/xray_logger.h"

static const char *XRAY_LABELS[XRAY_MODEL_OUTPUT_COUNT] = {
    "Atelectasis",
    "Consolidation",
    "Infiltration",
    "Pneumothorax",
    "Edema",
    "Emphysema",
    "Fibrosis",
    "Effusion",
    "Pneumonia",
    "Pleural_Thickening",
    "Cardiomegaly",
    "Nodule",
    "Mass",
    "Hernia",
    "Lung Lesion",
    "Fracture",
    "Lung Opacity",
    "Enlarged Cardiomediastinum"
};

static void rule_engine_copy_string(
    char *destination,
    size_t destination_size,
    const char *source
)
{
    if ((destination == NULL) || (destination_size == 0U) || (source == NULL))
    {
        return;
    }

    (void)strncpy(destination, source, destination_size - 1U);
    destination[destination_size - 1U] = '\0';
}

static const char *rule_engine_status_for_probability(
    float probability,
    const XrayRuleDefinition *rule
)
{
    if (probability < rule->low_threshold)
    {
        return "not suspected";
    }

    if (probability < rule->medium_threshold)
    {
        return "low suspicion";
    }

    if (probability < rule->high_threshold)
    {
        return "suspected";
    }

    return "strongly suspected";
}

static void rule_engine_note_for_status(
    const char *label,
    const char *status,
    char *note,
    size_t note_size
)
{
    if ((label == NULL) || (status == NULL) || (note == NULL) || (note_size == 0U))
    {
        return;
    }

    /*
     * MinGW/GCC can conservatively warn that label and note may overlap when
     * both originate from fields inside the same XrayAnalysisResult object.
     * Copy label to an independent local buffer before formatting the note.
     */
    char label_copy[XRAY_MAX_LABEL_LENGTH];

    (void)snprintf(
        label_copy,
        sizeof(label_copy),
        "%s",
        label
    );

    label = label_copy;

    if (strcmp(status, "not suspected") == 0)
    {
        (void)snprintf(
            note,
            note_size,
            "%s is not suspected by the model output. Doctor review remains required.",
            label
        );
    }
    else if (strcmp(status, "low suspicion") == 0)
    {
        (void)snprintf(
            note,
            note_size,
            "%s shows low model suspicion. Correlate with clinical context and physician review.",
            label
        );
    }
    else if (strcmp(status, "suspected") == 0)
    {
        (void)snprintf(
            note,
            note_size,
            "%s is suspected by the model output. Physician review is required before clinical use.",
            label
        );
    }
    else
    {
        (void)snprintf(
            note,
            note_size,
            "%s is strongly suspected by the model output. Prompt physician review is required.",
            label
        );
    }

    note[note_size - 1U] = '\0';
}

XrayStatus rule_engine_load_default_rules(
    XrayRuleSet *rule_set
)
{
    if (rule_set == NULL)
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    memset(rule_set, 0, sizeof(*rule_set));

    rule_set->rule_count = XRAY_MODEL_OUTPUT_COUNT;

    for (size_t i = 0U; i < XRAY_MODEL_OUTPUT_COUNT; ++i)
    {
        rule_set->rules[i].index = i;

        rule_engine_copy_string(
            rule_set->rules[i].label,
            sizeof(rule_set->rules[i].label),
            XRAY_LABELS[i]
        );

        rule_set->rules[i].low_threshold = 0.10F;
        rule_set->rules[i].medium_threshold = 0.30F;
        rule_set->rules[i].high_threshold = 0.60F;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "RULE_ENGINE",
        "Default rule set loaded: rules=%zu",
        rule_set->rule_count
    );

    return XRAY_STATUS_SUCCESS;
}

XrayStatus rule_engine_evaluate(
    const XrayRuleSet *rule_set,
    const float probabilities[XRAY_MODEL_OUTPUT_COUNT],
    XrayAnalysisResult *result
)
{
    if ((rule_set == NULL) || (probabilities == NULL) || (result == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    if (rule_set->rule_count != XRAY_MODEL_OUTPUT_COUNT)
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    result->finding_count = 0U;
    result->inference_completed = true;

    rule_engine_copy_string(
        result->warning,
        sizeof(result->warning),
        "AI-generated findings are doctor-assistive only. Physician review is required."
    );

    for (size_t i = 0U; i < XRAY_MODEL_OUTPUT_COUNT; ++i)
    {
        const float probability = probabilities[i];

        if (isnan(probability) || isinf(probability) ||
            (probability < 0.0F) || (probability > 1.0F))
        {
            xray_log(
                XRAY_LOG_LEVEL_ERROR,
                "RULE_ENGINE",
                "Invalid probability at index=%zu value=%f",
                i,
                (double)probability
            );

            return XRAY_STATUS_INVALID_ARGUMENT;
        }

        const XrayRuleDefinition *rule = &rule_set->rules[i];
        const char *status = rule_engine_status_for_probability(probability, rule);

        if (strcmp(status, "not suspected") == 0)
        {
            continue;
        }

        if (result->finding_count >= XRAY_MAX_FINDINGS)
        {
            return XRAY_STATUS_FAILURE;
        }

        FindingResult *finding = &result->findings[result->finding_count];

        rule_engine_copy_string(
            finding->label,
            sizeof(finding->label),
            rule->label
        );

        finding->probability = probability;

        rule_engine_copy_string(
            finding->status,
            sizeof(finding->status),
            status
        );

        rule_engine_note_for_status(
            finding->label,
            finding->status,
            finding->note,
            sizeof(finding->note)
        );

        finding->doctor_review_required = true;

        result->finding_count += 1U;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "RULE_ENGINE",
        "Rule evaluation completed: primary_findings=%zu",
        result->finding_count
    );

    return XRAY_STATUS_SUCCESS;
}
