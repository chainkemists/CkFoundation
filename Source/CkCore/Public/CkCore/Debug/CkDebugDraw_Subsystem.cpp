#include "CkDebugDraw_Subsystem.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkCore/Validation/CkIsValid.h"

#include <GameFramework/HUD.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DebugDraw_Subsystem_UE::
    Initialize(
        FSubsystemCollectionBase& InCollection)
    -> void
{
    Super::Initialize(InCollection);

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    { return; }

    _PostRenderHUD_DelegateHandle = AHUD::OnHUDPostRender.AddWeakLambda(this, [this](AHUD* InHUD, UCanvas* InCanvas)
    {
        if (ck::Is_NOT_Valid(this))
        { return; }

        this->DoOnPostRenderHUD(InHUD, InCanvas);
    });
}

auto
    UCk_DebugDraw_Subsystem_UE::
    Deinitialize()
    -> void
{
    Super::Deinitialize();

    if (GetWorld()->IsNetMode(NM_DedicatedServer) || GetWorld()->IsNetMode(NM_ListenServer))
    { return; }

    AHUD::OnHUDPostRender.Remove(_PostRenderHUD_DelegateHandle);
}

auto
    UCk_DebugDraw_Subsystem_UE::
    Request_DrawRect_OnScreen(
        const FCk_Request_DebugDrawOnScreen_Rect& InRequest)
    -> void
{
    _PendingDrawRequests.Emplace(InRequest);
}

auto
    UCk_DebugDraw_Subsystem_UE::
    Request_DrawLine_OnScreen(
        const FCk_Request_DebugDrawOnScreen_Line& InRequest)
    -> void
{
    _PendingDrawRequests.Emplace(InRequest);
}

void
    UCk_DebugDraw_Subsystem_UE::
    Request_DrawText_OnScreen(
        const FCk_Request_DebugDrawOnScreen_Text& InRequest)
{
    _PendingDrawRequests.Emplace(InRequest);
}

auto
    UCk_DebugDraw_Subsystem_UE::
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
    UCk_DebugDraw_Subsystem_UE::
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
    UCk_DebugDraw_Subsystem_UE::
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

auto
    UCk_DebugDraw_Subsystem_UE::
    DoHandleRequest(
        AHUD* InHUD,
        UCanvas* InCanvas,
        const FCk_Request_DebugDrawOnScreen_Text& InRequest)
        -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCanvas), TEXT("Canvas is not valid"))
    { return; }

    // Set text rendering properties
    const auto TextColor = InRequest.Get_TextColor().ToFColor(true);
    const auto TextScale = FVector2D(InRequest.Get_TextScale(), InRequest.Get_TextScale());

    // Use default font (you could make this configurable via the request if needed)
    auto Font = GEngine->GetMediumFont();
    CK_ENSURE_IF_NOT(ck::IsValid(Font), TEXT("Failed to get medium font"))
    { Font = GEngine->GetSmallFont(); } // Fallback to small font

    CK_ENSURE_IF_NOT(ck::IsValid(Font), TEXT("No valid font available for text rendering"))
    { return; }

    // Draw the text
    InCanvas->K2_DrawText(
        Font,
        InRequest.Get_Text(),
        InRequest.Get_TextPosition(),
        TextScale,
        TextColor,
        0.0f, // Kerning
        FLinearColor::Black, // Shadow color
        FVector2D(1.0f, 1.0f), // Shadow offset
        false, // Centered X
        false, // Centered Y
        true,  // Outlined
        FLinearColor::Black // Outline color
    );
}

// --------------------------------------------------------------------------------------------------------------------
