#include "engine/xray_engine.h"

#include <string.h>
#include <stdio.h>

#include "engine/image_loader.h"
#include "engine/preprocessor.h"
#include "engine/preprocessor_validation.h"
#include "engine/model_runner.h"
#include "engine/rule_engine.h"
#include "engine/model_output_validation.h"
#include "engine/xray_runtime_paths.h"
#include "export/result_formatter.h"
#include "db/database.h"
#include "utils/xray_logger.h"

static const char *xray_path_basename(const char *path)
{
    if (path == NULL)
    {
        return NULL;
    }

    const char *last_forward_slash = strrchr(path, '/');
    const char *last_backslash = strrchr(path, '\\');
    const char *basename = path;

    if ((last_forward_slash != NULL) && (last_backslash != NULL))
    {
        basename = (last_forward_slash > last_backslash)
            ? (last_forward_slash + 1)
            : (last_backslash + 1);
    }
    else if (last_forward_slash != NULL)
    {
        basename = last_forward_slash + 1;
    }
    else if (last_backslash != NULL)
    {
        basename = last_backslash + 1;
    }

    return basename;
}

static void xray_sanitize_filename_stem(char *stem)
{
    if (stem == NULL)
    {
        return;
    }

    size_t write_index = 0U;

    for (size_t read_index = 0U; stem[read_index] != '\0'; ++read_index)
    {
        char ch = stem[read_index];

        if ((ch == '<') || (ch == '>') ||
            (ch == ':') || (ch == '"') ||
            (ch == '/') || (ch == '\\') ||
            (ch == '|') || (ch == '?') ||
            (ch == '*'))
        {
            ch = '_';
        }

        stem[write_index] = ch;
        ++write_index;
    }

    stem[write_index] = '\0';

    if (stem[0] == '\0')
    {
        (void)strncpy(stem, "image", 6U);
    }
}

XrayStatus xray_engine_initialize(void)
{
    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Engine initialization started"
    );

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Engine initialization completed"
    );

    return XRAY_STATUS_SUCCESS;
}

XrayStatus xray_engine_shutdown(void)
{
    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Engine shutdown completed"
    );

    return XRAY_STATUS_SUCCESS;
}

XrayStatus xray_engine_analyze_image(
    const char *image_path,
    XrayAnalysisResult *result
)
{
    if ((image_path == NULL) || (result == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    memset(result, 0, sizeof(*result));

    (void)strncpy(result->study_type, "chest_xray", sizeof(result->study_type) - 1U);
    (void)strncpy(result->image_path, image_path, sizeof(result->image_path) - 1U);

    result->inference_completed = false;

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Analyze image request received: %s",
        image_path
    );

    XrayImage image;

    XrayStatus status = image_loader_load_grayscale(image_path, &image);

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Image loading failed: %s",
            xray_status_to_string(status)
        );

        return status;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Image loading completed: width=%zu height=%zu channels=%zu",
        image.width,
        image.height,
        image.channels
    );

    XrayTensor tensor;

    status = preprocessor_create_tensor(&image, &tensor);

    image_loader_free(&image);

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Image resources released"
    );

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Preprocessing failed: %s",
            xray_status_to_string(status)
        );

        return status;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Tensor preprocessing completed"
    );

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Tensor shape: [%zu,%zu,%zu,%zu]",
        tensor.batch,
        tensor.channels,
        tensor.height,
        tensor.width
    );

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Tensor elements: %zu",
        tensor.total_elements
    );

    const char *image_filename = xray_path_basename(image_path);

    char image_stem[128] = {0};

    const char *extension = strrchr(image_filename, '.');

    size_t stem_length = 0U;

    if (extension == NULL)
    {
        stem_length = strlen(image_filename);
    }
    else
    {
        stem_length = (size_t)(extension - image_filename);
    }

    if (stem_length >= sizeof(image_stem))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Image filename too long for tensor export"
        );

        preprocessor_free_tensor(&tensor);
        return XRAY_STATUS_FAILURE;
    }

    memcpy(image_stem, image_filename, stem_length);
    image_stem[stem_length] = '\0';
    xray_sanitize_filename_stem(image_stem);

    char tensor_output_path[XRAY_MAX_PATH_LENGTH];

    const int path_chars = snprintf(
        tensor_output_path,
        sizeof(tensor_output_path),
        XRAY_PREPROCESSING_VALIDATION_DIRECTORY "/c_tensor_%s.bin",
        image_stem
    );

    if ((path_chars < 0) || ((size_t)path_chars >= sizeof(tensor_output_path)))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Tensor export path generation failed"
        );

        preprocessor_free_tensor(&tensor);
        return XRAY_STATUS_FAILURE;
    }

    status = preprocessor_validation_write_tensor_binary(
        &tensor,
        tensor_output_path
    );

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Tensor validation export failed: %s",
            xray_status_to_string(status)
        );

        preprocessor_free_tensor(&tensor);
        return status;
    }

    ModelRunner *runner = NULL;

    status = model_runner_create(XRAY_MODEL_PATH, &runner);

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Model runner creation failed: %s",
            xray_status_to_string(status)
        );

        preprocessor_free_tensor(&tensor);
        return status;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Model runner created successfully"
    );

    float probabilities[XRAY_MODEL_OUTPUT_COUNT] = {0.0F};

    status = model_runner_run(runner, &tensor, probabilities);

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Model inference failed: %s",
            xray_status_to_string(status)
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Model inference completed"
    );

    for (size_t i = 0U; i < XRAY_MODEL_OUTPUT_COUNT; ++i)
    {
        xray_log(
            XRAY_LOG_LEVEL_INFO,
            "ENGINE",
            "Probability[%zu]=%.6f",
            i,
            (double)probabilities[i]
        );
    }

    char probability_output_path[XRAY_MAX_PATH_LENGTH];

    const int probability_path_chars = snprintf(
        probability_output_path,
        sizeof(probability_output_path),
        XRAY_ONNX_VALIDATION_DIRECTORY "/c_probabilities_%s.bin",
        image_stem
    );

    if ((probability_path_chars < 0) ||
        ((size_t)probability_path_chars >= sizeof(probability_output_path)))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Probability export path generation failed"
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return XRAY_STATUS_FAILURE;
    }

    status = model_output_validation_write_probabilities_binary(
        probabilities,
        probability_output_path
    );

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Probability validation export failed: %s",
            xray_status_to_string(status)
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    XrayRuleSet rule_set;

    status = rule_engine_load_default_rules(&rule_set);

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Rule set loading failed: %s",
            xray_status_to_string(status)
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    XrayAnalysisResult analysis_result;

    memset(&analysis_result, 0, sizeof(analysis_result));

    (void)snprintf(
        analysis_result.study_type,
        sizeof(analysis_result.study_type),
        "%s",
        "chest_xray"
    );

    (void)snprintf(
        analysis_result.image_path,
        sizeof(analysis_result.image_path),
        "%s",
        image_path
    );

    status = rule_engine_evaluate(
        &rule_set,
        probabilities,
        &analysis_result
    );

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Rule evaluation failed: %s",
            xray_status_to_string(status)
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Clinical findings generated: count=%zu",
        analysis_result.finding_count
    );

    for (size_t i = 0U; i < analysis_result.finding_count; ++i)
    {
        const FindingResult *finding = &analysis_result.findings[i];

        xray_log(
            XRAY_LOG_LEVEL_INFO,
            "ENGINE",
            "Finding[%zu]: label=%s probability=%.6f status=%s",
            i,
            finding->label,
            (double)finding->probability,
            finding->status
        );
    }

    result_formatter_print_console(&analysis_result);

    char report_output_path[XRAY_MAX_PATH_LENGTH];

    const int report_path_chars = snprintf(
        report_output_path,
        sizeof(report_output_path),
        XRAY_REPORT_DIRECTORY "/report_%s.json",
        image_stem
    );

    if ((report_path_chars < 0) ||
        ((size_t)report_path_chars >= sizeof(report_output_path)))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Report output path generation failed"
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return XRAY_STATUS_FAILURE;
    }

    status = result_formatter_write_json(
        &analysis_result,
        report_output_path
    );

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "JSON report generation failed: %s",
            xray_status_to_string(status)
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    XrayDatabase database;

    status = database_initialize(
        &database,
        XRAY_DATABASE_PATH
    );

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Database initialization failed: %s",
            xray_status_to_string(status)
        );

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    status = database_create_schema(&database);

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Database schema creation failed: %s",
            xray_status_to_string(status)
        );

        database_close(&database);

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    status = database_store_analysis_result(
        &database,
        &analysis_result
    );

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "ENGINE",
            "Database persistence failed: %s",
            xray_status_to_string(status)
        );

        database_close(&database);

        model_runner_destroy(runner);
        preprocessor_free_tensor(&tensor);
        return status;
    }

    database_close(&database);

    memcpy(result, &analysis_result, sizeof(*result));

    model_runner_destroy(runner);

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Model runner destroyed"
    );

    preprocessor_free_tensor(&tensor);

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "ENGINE",
        "Tensor resources released"
    );

    return XRAY_STATUS_SUCCESS;
}
