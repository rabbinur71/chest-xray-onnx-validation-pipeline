#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "engine/xray_engine.h"
#include "engine/xray_types.h"
#include "utils/xray_logger.h"
#include "engine/xray_runtime_paths.h"

static void print_usage(const char *program_name)
{
    fprintf(
        stderr,
        "Usage: %s <image_path>\n\n"
        "Offline AI-assisted chest X-ray analysis CLI.\n\n"
        "Arguments:\n"
        "  image_path    Path to JPG/JPEG/PNG chest X-ray image\n\n"
        "Examples:\n"
        "  %s validation_dataset/cardiomegaly/cardiomegaly_001.jpg\n",
        program_name,
        program_name
    );
}

static int file_exists(const char *path)
{
    if (path == NULL)
    {
        return 0;
    }

    return access(path, R_OK) == 0;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if ((strcmp(argv[1], "--help") == 0) ||
        (strcmp(argv[1], "-h") == 0))
    {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    const char *image_path = argv[1];

    xray_log(XRAY_LOG_LEVEL_INFO, "CLI", "X-Ray Doctor Assist CLI initialized");
    xray_log(XRAY_LOG_LEVEL_INFO, "CLI", "Operational mode: offline inference");
    xray_log(XRAY_LOG_LEVEL_INFO, "CLI", "Input image path: %s", image_path);

    if (!file_exists(XRAY_MODEL_PATH))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "CLI",
            "Required model file is missing or unreadable: %s",
            XRAY_MODEL_PATH
        );

        return EXIT_FAILURE;
    }

    if (!file_exists(image_path))
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "CLI",
            "Input image file is missing or unreadable: %s",
            image_path
        );

        return EXIT_FAILURE;
    }

    XrayStatus status = xray_engine_initialize();

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "CLI",
            "Engine initialization failed: %s",
            xray_status_to_string(status)
        );

        return EXIT_FAILURE;
    }

    XrayAnalysisResult result;

    status = xray_engine_analyze_image(image_path, &result);

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "CLI",
            "Image analysis failed: %s",
            xray_status_to_string(status)
        );

        (void)xray_engine_shutdown();
        return EXIT_FAILURE;
    }

    xray_log(
        XRAY_LOG_LEVEL_INFO,
        "CLI",
        "Image analysis completed successfully"
    );

    status = xray_engine_shutdown();

    if (status != XRAY_STATUS_SUCCESS)
    {
        xray_log(
            XRAY_LOG_LEVEL_ERROR,
            "CLI",
            "Engine shutdown failed: %s",
            xray_status_to_string(status)
        );

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
