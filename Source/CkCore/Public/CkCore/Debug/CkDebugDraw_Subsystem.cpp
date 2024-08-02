#include "CkDebugDraw_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkCore/Validation/CkIsValid.h"

#include <GameFramework/HUD.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    _PostRenderHUD_DelegateHandle = AHUD::OnHUDPostRender.AddWeakLambda(this, [this](AHUD* InHUD, UCanvas* InCanvas)
    {
        if (ck::Is_NOT_Valid(this))
        { return; }

        this->DoOnPostRenderHUD(InHUD, InCanvas);
    });
}

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    AHUD::OnHUDPostRender.Remove(_PostRenderHUD_DelegateHandle);
}

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    Request_DrawRect_OnScreen(
        const FCk_Request_DebugDrawOnScreen_Rect& InRequest)
    -> void
{
    _PendingDrawRequests.Emplace(InRequest);
}

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    Request_DrawLine_OnScreen(
        const FCk_Request_DebugDrawOnScreen_Line& InRequest)
    -> void
{
    _PendingDrawRequests.Emplace(InRequest);
}

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    DoOnPostRenderHUD(
        AHUD* InHUD,
        UCanvas* InCanvas)
    -> void
{
    auto DrawRequests = decltype(_PendingDrawRequests){};
    Swap(_PendingDrawRequests, DrawRequests);

    if (ck::Is_NOT_Valid(InHUD))
    { return; }

    ck::algo::ForEachRequest(DrawRequests, ck::Visitor([&](const auto& RequestVariant)
    {
        DoHandleRequest(InHUD, InCanvas, RequestVariant);
    }));
}

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    DoHandleRequest(
        AHUD* InHUD,
        UCanvas* InCanvas,
        const FCk_Request_DebugDrawOnScreen_Rect& InRequest)
    -> void
{
    const auto& Rect = InRequest.Get_Rect();
    const auto& RectColor = InRequest.Get_RectColor();

    InHUD->DrawRect(RectColor, Rect.Min.X, Rect.Min.Y, Rect.GetExtent().X * 2.0f, Rect.GetExtent().Y * 2.0f);
}

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    DoHandleRequest(
        AHUD* InHUD,
        UCanvas* InCanvas,
        const FCk_Request_DebugDrawOnScreen_Line& InRequest)
    -> void
{
    const auto& LineStart = InRequest.Get_LineStart();
    const auto& LineEnd = InRequest.Get_LineEnd();
    const auto& LineColor = InRequest.Get_LineColor();
    const auto& LineThickness = InRequest.Get_LineThickness();

    InHUD->DrawLine(LineStart.X, LineStart.Y, LineEnd.X, LineEnd.Y, LineColor, LineThickness);
}

// --------------------------------------------------------------------------------------------------------------------
