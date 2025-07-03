#pragma once
#include <d3d11.h>

namespace PBBypass {
    void Initialize(ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swapChain);
    void Shutdown();
    void OnFrame();
    void SetEnabled(bool enabled);
    bool IsEnabled();
} 