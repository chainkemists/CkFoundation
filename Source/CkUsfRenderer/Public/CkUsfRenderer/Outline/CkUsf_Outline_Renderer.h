#pragma once

#include "CoreMinimal.h"
#include "Containers/StaticArray.h"

class UWorld;

namespace ck::usf
{
    // Plain immutable snapshots cross the game/render-thread boundary. No UObject or ECS handles
    // reach the renderer. CkUsf validates settings before publishing one of these snapshots.
    struct FOutlineRenderState
    {
        TStaticArray<FVector4f, 16> Outline{};
        TStaticArray<FVector4f, 16> Fill{};
        uint32 ActiveMask = 0;
        uint32 StencilMin = 240;
        bool WorldSpace = true;
        bool SquareCorners = true;
        float Thickness = 5.0f;
    };

    class FOutlineViewExtension;

    class CKUSFRENDERER_API FOutlineRenderer
    {
    public:
        explicit FOutlineRenderer(UWorld* InWorld);
        ~FOutlineRenderer();
        auto Set_State(const FOutlineRenderState& InState) -> void;
        auto Deactivate() -> void;

    private:
        TSharedPtr<FOutlineViewExtension, ESPMode::ThreadSafe> _Extension;
    };
}
