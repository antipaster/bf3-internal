#include "PBBypass.h"
#include <Windows.h>
#include <d3d11.h>
#include <atomic>
#include <mutex>
#include <string>
#include <chrono>
#include <filesystem>
#include <fstream>

static bool g_enabled = false;
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_context = nullptr;
static IDXGISwapChain* g_swapChain = nullptr;
static ID3D11Texture2D* g_cleanTexture = nullptr;
static std::mutex g_mutex;


using CopySubresourceRegion_t = void (STDMETHODCALLTYPE *)(ID3D11DeviceContext*, ID3D11Resource*, UINT, UINT, UINT, UINT, ID3D11Resource*, UINT, const D3D11_BOX*);
static CopySubresourceRegion_t oCopySubresourceRegion = nullptr;


static std::string GetScreenshotFilename() {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    struct tm buf;
    localtime_s(&buf, &t);
    char filename[128];
    strftime(filename, sizeof(filename), "PBSS_%Y%m%d_%H%M%S.bmp", &buf);
    return filename;
}


static void SaveTextureToBMP(ID3D11Texture2D* texture, const char* filename) {
    if (!texture) return;
    D3D11_TEXTURE2D_DESC desc;
    texture->GetDesc(&desc);
    if (desc.Format != DXGI_FORMAT_R8G8B8A8_UNORM) return; // Only support this format for now

    ID3D11Device* device = nullptr;
    texture->GetDevice(&device);
    ID3D11DeviceContext* context = nullptr;
    device->GetImmediateContext(&context);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(context->Map(texture, 0, D3D11_MAP_READ, 0, &mapped))) {
        std::ofstream file(filename, std::ios::binary);
        if (file) {
            // Write BMP header
            int w = desc.Width, h = desc.Height;
            int rowSize = ((w * 3 + 3) & ~3);
            int dataSize = rowSize * h;
            int fileSize = 54 + dataSize;
            unsigned char header[54] = {
                'B','M', fileSize,0,0,0, 0,0,0,0, 54,0,0,0, 40,0,0,0,
                w,0,0,0, h,0,0,0, 1,0,24,0, 0,0,0,0, dataSize,0,0,0,
                0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0
            };
            file.write((char*)header, 54);
    
            for (int y = h - 1; y >= 0; --y) {
                unsigned char* row = (unsigned char*)mapped.pData + mapped.RowPitch * y;
                for (int x = 0; x < w; ++x) {
                    file.put(row[x * 4 + 2]); // B
                    file.put(row[x * 4 + 1]); // G
                    file.put(row[x * 4 + 0]); // R
                }
                for (int pad = w * 3; pad < rowSize; ++pad) file.put(0);
            }
        }
        context->Unmap(texture, 0);
    }
    if (context) context->Release();
    if (device) device->Release();
}


void STDMETHODCALLTYPE hkCopySubresourceRegion(ID3D11DeviceContext* ctx, ID3D11Resource* dst, UINT dstSub, UINT dstX, UINT dstY, UINT dstZ, ID3D11Resource* src, UINT srcSub, const D3D11_BOX* box) {
    if (g_enabled && g_cleanTexture) {
        std::lock_guard<std::mutex> lock(g_mutex);
        std::string filename = GetScreenshotFilename();
        SaveTextureToBMP(g_cleanTexture, filename.c_str());
        oCopySubresourceRegion(ctx, dst, dstSub, dstX, dstY, dstZ, g_cleanTexture, srcSub, box);
        return;
    }
    oCopySubresourceRegion(ctx, dst, dstSub, dstX, dstY, dstZ, src, srcSub, box);
}

void PBBypass::Initialize(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain) {
    g_device = device;
    g_context = context;
    g_swapChain = swapChain;
    void** vtable = *(void***)(context);
    oCopySubresourceRegion = (CopySubresourceRegion_t)vtable[46];
    DWORD oldProtect;
    VirtualProtect(&vtable[46], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
    vtable[46] = (void*)&hkCopySubresourceRegion;
    VirtualProtect(&vtable[46], sizeof(void*), oldProtect, &oldProtect);
}

void PBBypass::Shutdown() {
    if (g_cleanTexture) { g_cleanTexture->Release(); g_cleanTexture = nullptr; }
}

void PBBypass::OnFrame() {
    if (!g_enabled || !g_swapChain || !g_device || !g_context) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    ID3D11Texture2D* backBuffer = nullptr;
    if (SUCCEEDED(g_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer))) {
        D3D11_TEXTURE2D_DESC desc;
        backBuffer->GetDesc(&desc);
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;
        if (g_cleanTexture) g_cleanTexture->Release();
        g_device->CreateTexture2D(&desc, nullptr, &g_cleanTexture);
        g_context->CopyResource(g_cleanTexture, backBuffer);
        backBuffer->Release();
    }
}

void PBBypass::SetEnabled(bool enabled) { g_enabled = enabled; }
bool PBBypass::IsEnabled() { return g_enabled; } 