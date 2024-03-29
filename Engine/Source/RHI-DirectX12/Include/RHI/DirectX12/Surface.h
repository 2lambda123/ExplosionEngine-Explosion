//
// Created by johnk on 2023/4/17.
//

#pragma once

#include <Windows.h>

#include <RHI/Surface.h>

namespace RHI::DirectX12 {
    class DX12Surface : public Surface {
    public:
        NonCopyable(DX12Surface)
        explicit DX12Surface(const SurfaceCreateInfo& createInfo);
        ~DX12Surface() override;

        void Destroy() override;
        HWND GetWin32WindowHandle() const;

    private:
        HWND hWnd;
    };
}
