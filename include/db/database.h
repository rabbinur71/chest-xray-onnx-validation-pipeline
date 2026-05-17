#ifndef DATABASE_H
#define DATABASE_H

#include "engine/xray_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    void *handle;
} XrayDatabase;

XrayStatus database_initialize(
    XrayDatabase *database,
    const char *database_path
);

XrayStatus database_create_schema(
    XrayDatabase *database
);

XrayStatus database_store_analysis_result(
    XrayDatabase *database,
    const XrayAnalysisResult *result
);

void database_close(
    XrayDatabase *database
);

#ifdef __cplusplus
}
#endif

#endif /* DATABASE_H */
