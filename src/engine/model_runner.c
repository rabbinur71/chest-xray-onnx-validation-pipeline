#include "engine/model_runner.h"

#include <onnxruntime_c_api.h>

#include <stdlib.h>

#ifdef _WIN32
#include <wchar.h>
#endif

#include "utils/xray_logger.h"

struct ModelRunner
{
    const OrtApi *api;
    OrtEnv *env;
    OrtSessionOptions *session_options;
    OrtSession *session;
    OrtMemoryInfo *memory_info;
};

static XrayStatus model_runner_check_ort_status(
    const OrtApi *api,
    OrtStatus *status,
    const char *operation
)
{
    if (status == NULL)
    {
        return XRAY_STATUS_SUCCESS;
    }

    const char *message = api->GetErrorMessage(status);

    xray_log(
        XRAY_LOG_LEVEL_ERROR,
        "MODEL_RUNNER",
        "ONNX Runtime failure during %s: %s",
        operation,
        message
    );

    api->ReleaseStatus(status);

    return XRAY_STATUS_MODEL_LOAD_FAILED;
}

XrayStatus model_runner_create(
    const char *model_path,
    ModelRunner **runner
)
{
    if ((model_path == NULL) || (runner == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    *runner = NULL;

    ModelRunner *instance = calloc(1U, sizeof(ModelRunner));

    if (instance == NULL)
    {
        return XRAY_STATUS_FAILURE;
    }

    instance->api = OrtGetApiBase()->GetApi(ORT_API_VERSION);

    if (instance->api == NULL)
    {
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

    OrtStatus *status = instance->api->CreateEnv(
        ORT_LOGGING_LEVEL_WARNING,
        "xray-doctor-assist",
        &instance->env
    );

    if (model_runner_check_ort_status(instance->api, status, "CreateEnv") != XRAY_STATUS_SUCCESS)
    {
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

    status = instance->api->CreateSessionOptions(&instance->session_options);

    if (model_runner_check_ort_status(instance->api, status, "CreateSessionOptions") != XRAY_STATUS_SUCCESS)
    {
        instance->api->ReleaseEnv(instance->env);
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

    status = instance->api->SetIntraOpNumThreads(
        instance->session_options,
        1
    );

    if (model_runner_check_ort_status(instance->api, status, "SetIntraOpNumThreads") != XRAY_STATUS_SUCCESS)
    {
        instance->api->ReleaseSessionOptions(instance->session_options);
        instance->api->ReleaseEnv(instance->env);
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

    status = instance->api->SetSessionGraphOptimizationLevel(
        instance->session_options,
        ORT_ENABLE_BASIC
    );

    if (model_runner_check_ort_status(instance->api, status, "SetSessionGraphOptimizationLevel") != XRAY_STATUS_SUCCESS)
    {
        instance->api->ReleaseSessionOptions(instance->session_options);
        instance->api->ReleaseEnv(instance->env);
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

#ifdef _WIN32
    wchar_t wide_model_path[512];

    const size_t converted_chars = mbstowcs(
        wide_model_path,
        model_path,
        (sizeof(wide_model_path) / sizeof(wide_model_path[0])) - 1U
    );

    if (converted_chars == (size_t)-1)
    {
        instance->api->ReleaseSessionOptions(instance->session_options);
        instance->api->ReleaseEnv(instance->env);
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

    wide_model_path[converted_chars] = L'\0';

    status = instance->api->CreateSession(
        instance->env,
        wide_model_path,
        instance->session_options,
        &instance->session
    );
#else
    status = instance->api->CreateSession(
        instance->env,
        model_path,
        instance->session_options,
        &instance->session
    );
#endif

    if (model_runner_check_ort_status(instance->api, status, "CreateSession") != XRAY_STATUS_SUCCESS)
    {
        instance->api->ReleaseSessionOptions(instance->session_options);
        instance->api->ReleaseEnv(instance->env);
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

    status = instance->api->CreateCpuMemoryInfo(
        OrtArenaAllocator,
        OrtMemTypeDefault,
        &instance->memory_info
    );

    if (model_runner_check_ort_status(instance->api, status, "CreateCpuMemoryInfo") != XRAY_STATUS_SUCCESS)
    {
        instance->api->ReleaseSession(instance->session);
        instance->api->ReleaseSessionOptions(instance->session_options);
        instance->api->ReleaseEnv(instance->env);
        free(instance);
        return XRAY_STATUS_MODEL_LOAD_FAILED;
    }

    *runner = instance;

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "MODEL_RUNNER",
        "ONNX Runtime model loaded: %s",
        model_path
    );

    return XRAY_STATUS_SUCCESS;
}

XrayStatus model_runner_run(
    ModelRunner *runner,
    const XrayTensor *input_tensor,
    float output_probabilities[XRAY_MODEL_OUTPUT_COUNT]
)
{
    if ((runner == NULL) ||
        (input_tensor == NULL) ||
        (input_tensor->data == NULL) ||
        (output_probabilities == NULL))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    if (input_tensor->total_elements != (1U * 1U * 224U * 224U))
    {
        return XRAY_STATUS_INVALID_ARGUMENT;
    }

    int64_t input_shape[4] = {
        (int64_t)input_tensor->batch,
        (int64_t)input_tensor->channels,
        (int64_t)input_tensor->height,
        (int64_t)input_tensor->width
    };

    OrtValue *input_value = NULL;

    OrtStatus *status = runner->api->CreateTensorWithDataAsOrtValue(
        runner->memory_info,
        input_tensor->data,
        input_tensor->total_elements * sizeof(float),
        input_shape,
        4U,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        &input_value
    );

    if (model_runner_check_ort_status(runner->api, status, "CreateTensorWithDataAsOrtValue") != XRAY_STATUS_SUCCESS)
    {
        return XRAY_STATUS_INFERENCE_FAILED;
    }

    const char *input_names[] = {"input_image"};
    const char *output_names[] = {"pathology_probabilities"};

    OrtValue *output_value = NULL;

    status = runner->api->Run(
        runner->session,
        NULL,
        input_names,
        (const OrtValue *const *)&input_value,
        1U,
        output_names,
        1U,
        &output_value
    );

    runner->api->ReleaseValue(input_value);

    if (model_runner_check_ort_status(runner->api, status, "Run") != XRAY_STATUS_SUCCESS)
    {
        return XRAY_STATUS_INFERENCE_FAILED;
    }

    float *output_data = NULL;

    status = runner->api->GetTensorMutableData(
        output_value,
        (void **)&output_data
    );

    if (model_runner_check_ort_status(runner->api, status, "GetTensorMutableData") != XRAY_STATUS_SUCCESS)
    {
        runner->api->ReleaseValue(output_value);
        return XRAY_STATUS_INFERENCE_FAILED;
    }

    for (size_t i = 0U; i < XRAY_MODEL_OUTPUT_COUNT; ++i)
    {
        output_probabilities[i] = output_data[i];
    }

    runner->api->ReleaseValue(output_value);

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "MODEL_RUNNER",
        "ONNX inference completed: outputs=%u",
        XRAY_MODEL_OUTPUT_COUNT
    );

    return XRAY_STATUS_SUCCESS;
}

void model_runner_destroy(ModelRunner *runner)
{
    if (runner == NULL)
    {
        return;
    }

    if (runner->api != NULL)
    {
        if (runner->memory_info != NULL)
        {
            runner->api->ReleaseMemoryInfo(runner->memory_info);
        }

        if (runner->session != NULL)
        {
            runner->api->ReleaseSession(runner->session);
        }

        if (runner->session_options != NULL)
        {
            runner->api->ReleaseSessionOptions(runner->session_options);
        }

        if (runner->env != NULL)
        {
            runner->api->ReleaseEnv(runner->env);
        }
    }

    free(runner);
}
