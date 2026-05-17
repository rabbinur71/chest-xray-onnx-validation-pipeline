#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "engine/rule_engine.h"
#include "engine/xray_types.h"

static int run_invalid_case(const char *case_name, float invalid_value)
{
    XrayRuleSet rule_set;
    XrayAnalysisResult result;
    float probabilities[XRAY_MODEL_OUTPUT_COUNT];

    for (size_t i = 0U; i < XRAY_MODEL_OUTPUT_COUNT; ++i)
    {
        probabilities[i] = 0.0F;
    }

    probabilities[0] = invalid_value;

    memset(&result, 0, sizeof(result));

    XrayStatus status = rule_engine_load_default_rules(&rule_set);

    if (status != XRAY_STATUS_SUCCESS)
    {
        fprintf(stderr, "[FAIL] %s: failed to load rules\n", case_name);
        return EXIT_FAILURE;
    }

    status = rule_engine_evaluate(&rule_set, probabilities, &result);

    if (status != XRAY_STATUS_INVALID_ARGUMENT)
    {
        fprintf(
            stderr,
            "[FAIL] %s: expected INVALID_ARGUMENT, got %s\n",
            case_name,
            xray_status_to_string(status)
        );

        return EXIT_FAILURE;
    }

    printf("[PASS] %s rejected correctly\n", case_name);
    return EXIT_SUCCESS;
}

int main(void)
{
    if (run_invalid_case("NaN probability", NAN) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    if (run_invalid_case("positive infinity probability", INFINITY) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    if (run_invalid_case("negative probability", -0.01F) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    if (run_invalid_case("above-one probability", 1.01F) != EXIT_SUCCESS)
    {
        return EXIT_FAILURE;
    }

    printf("[PASS] Rule engine invalid probability safety validation complete\n");

    return EXIT_SUCCESS;
}
