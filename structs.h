#ifndef STRUCTS_H
#define STRUCTS_H

#include <windef.h>
#include <windows.h>

enum class WindowCompositionAttribute {
    AccentPolicy = 19
};

struct WindowCompositionAttributeData {
    WindowCompositionAttribute attribute;
    PVOID data;
    SIZE_T dataSize;
};

enum class AccentEffect {
    Disabled = 0,
    EnableGradient = 1,
    EnableTransparentGradient = 2,
    EnableBlurBehind = 3,
    EnableAcrylicBlur = 4,
    EnableHostBackdrop = 5,
    Invalid = 6
};

struct AccentEffectConfig {
    AccentEffect effectType;
    DWORD flags;
    DWORD color;
    DWORD animationId;
};

using SetWindowCompositionAttributeFunc = BOOL(WINAPI*)(HWND, WindowCompositionAttributeData*);

#endif // STRUCTS_H
