#pragma once
#include <d2d1.h>
#include <wrl/client.h>
#include <vector>
#include <memory>

struct AtlasEntry {
    int x = 0, y = 0;
    int width = 0, height = 0;
    int originalWidth = 0, originalHeight = 0;
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    int frameIndex = 0; // For animated icons
};

class TextureAtlas {
public:
    bool Initialize(ID2D1RenderTarget* rt, int maxSize = 1024);
    bool AddBitmap(ID2D1Bitmap* bitmap, AtlasEntry& outEntry);
    bool Pack();
    void Clear();
    ID2D1Bitmap* GetAtlasBitmap() const { return m_atlasBitmap.Get(); }
    int GetAtlasSize() const { return m_atlasSize; }
    bool IsInitialized() const { return m_atlasBitmap != nullptr; }
private:
    Microsoft::WRL::ComPtr<ID2D1RenderTarget> m_renderTarget;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_atlasBitmap;
    std::vector<AtlasEntry> m_entries;
    std::vector<Microsoft::WRL::ComPtr<ID2D1Bitmap>> m_pendingBitmaps;
    int m_atlasSize = 1024;
    int m_currentX = 0, m_currentY = 0;
    int m_rowHeight = 0;
    bool m_dirty = false;

    bool RebuildAtlas();
};
