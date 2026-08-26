#pragma once
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>

#define DF_PLUGIN_API_VERSION 1

struct DFPluginContext {
    ID2D1RenderTarget* renderTarget = nullptr;
    IDWriteFactory* writeFactory = nullptr;
    HWND dockHwnd = nullptr;
    void* userData = nullptr;
};

struct DFPluginVTable {
    int apiVersion = DF_PLUGIN_API_VERSION;
    const char* name = nullptr;
    const char* version = nullptr;
    bool (*Initialize)(DFPluginContext* ctx) = nullptr;
    void (*Shutdown)() = nullptr;
    void (*Update)(float deltaTime) = nullptr;
    void (*Render)(ID2D1RenderTarget* rt, IDWriteFactory* wf, float x, float y, float w, float h) = nullptr;
    void (*OnClick)(float localX, float localY) = nullptr;
    float preferredWidth = 64.0f;
    float preferredHeight = 48.0f;
};

typedef DFPluginVTable* (*DFGetPluginFunc)();