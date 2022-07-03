#pragma once

#define PYPLUGIN_EXPORT __declspec(dllexport)

extern "C" PYPLUGIN_EXPORT void Initialize();
extern "C" PYPLUGIN_EXPORT void Finalize();
