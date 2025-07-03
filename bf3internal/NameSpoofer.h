#pragma once
#include <string>

namespace NameSpoofer {
    void SpoofName(const char* newName);
    void RestoreName();
    void OnFrame();
    void SetEnabled(bool enabled);
    bool IsEnabled();
    void SetName(const char* name);
    const char* GetOriginalName();
} 