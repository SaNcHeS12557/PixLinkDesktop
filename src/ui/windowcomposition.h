#ifndef WINDOWCOMPOSITION_H
#define WINDOWCOMPOSITION_H

#include <windows.h>

// Types for SetWindowCompositionAttribute, an undocumented user32 function that gives a window the
// acrylic blur-behind effect. It is not in the Windows SDK headers, so it is declared here and loaded
// at runtime with GetProcAddress (see MainWindow::applyAcrylicBlur).

enum class WindowCompositionAttribute
{
    AccentPolicy = 19,
};

struct WindowCompositionAttributeData
{
    WindowCompositionAttribute attribute;
    PVOID data;
    SIZE_T dataSize;
};

enum class AccentEffect
{
    Disabled = 0,
    EnableGradient = 1,
    EnableTransparentGradient = 2,
    EnableBlurBehind = 3,
    EnableAcrylicBlur = 4,
    EnableHostBackdrop = 5,
    Invalid = 6,
};

struct AccentEffectConfig
{
    AccentEffect effectType;
    DWORD flags;
    DWORD color; // tint in 0xAABBGGRR order
    DWORD animationId;
};

using SetWindowCompositionAttributeFunc = BOOL(WINAPI *)(HWND, WindowCompositionAttributeData *);

#endif // WINDOWCOMPOSITION_H
