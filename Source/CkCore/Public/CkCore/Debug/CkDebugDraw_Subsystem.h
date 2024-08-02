#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include "CkDebugDraw_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_Request_DebugDrawOnScreen_Rect
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DebugDrawOnScreen_Rect);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FBox2D _Rect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _RectColor = FLinearColor::White;

public:
    CK_PROPERTY_GET(_Rect);
    CK_PROPERTY(_RectColor);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_DebugDrawOnScreen_Rect, _Rect);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCORE_API UCk_DebugDraw_WorldSubsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DebugDraw_WorldSubsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& InCollection) -> void override;

    auto
    Deinitialize() -> void override;

public:
    UFUNCTION(BlueprintCallable)
    void
    Request_DrawRect_OnScreen(
        const FCk_Request_DebugDrawOnScreen_Rect& InRequest);

private:
    auto
    DoOnPostRenderHUD(
        AHUD* InHUD,
        UCanvas* InCanvas) -> void;

private:
    UPROPERTY(Transient)
    TArray<FCk_Request_DebugDrawOnScreen_Rect> _PendingRequests_DrawRect;

    FDelegateHandle _PostRenderHUD_DelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------
