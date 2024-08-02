#include "CkDebugDraw_Subsystem.h"

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
    _PendingRequests_DrawRect.Add(InRequest);
}

auto
    UCk_DebugDraw_WorldSubsystem_UE::
    DoOnPostRenderHUD(
        AHUD* InHUD,
        UCanvas* InCanvas)
    -> void
{
    auto DrawRectRequests = TArray<FCk_Request_DebugDrawOnScreen_Rect>{};
    Swap(_PendingRequests_DrawRect, DrawRectRequests);

    if (ck::Is_NOT_Valid(InHUD))
    { return; }

    for (const auto& Request : DrawRectRequests)
    {
        const auto& Rect = Request.Get_Rect();
        const auto& RectColor = Request.Get_RectColor();

        InHUD->DrawRect(RectColor, Rect.Min.X, Rect.Min.Y, Rect.GetExtent().X * 2.0f, Rect.GetExtent().Y * 2.0f);
    }
}

// --------------------------------------------------------------------------------------------------------------------
