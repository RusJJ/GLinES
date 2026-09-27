#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void* GLIN_GetProcAddress(const char* name);
void GLIN_SetProcAddr(void* (*resolver)(const char*));
void* gl4es_GetProcAddress(const char* name);
void initialize_gl4es(void);

#ifdef __cplusplus
}
#endif
