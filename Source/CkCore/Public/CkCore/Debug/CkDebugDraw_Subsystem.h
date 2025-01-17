#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"

#include <variant>

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

USTRUCT(BlueprintType)
struct FCk_Request_DebugDrawOnScreen_Line
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DebugDrawOnScreen_Line);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _LineStart;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FVector2D _LineEnd;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FLinearColor _LineColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    float _LineThickness = 1.0f;

public:
    CK_PROPERTY_GET(_LineStart);
    CK_PROPERTY_GET(_LineEnd);
    CK_PROPERTY(_LineColor);
    CK_PROPERTY(_LineThickness);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_DebugDrawOnScreen_Line, _LineStart, _LineEnd);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_DebugDraw")
class CKCORE_API UCk_DebugDraw_Subsystem_UE : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DebugDraw_Subsystem_UE);

public:
    using DrawRectRequestType = FCk_Request_DebugDrawOnScreen_Rect;
    using DrawLineRequestType = FCk_Request_DebugDrawOnScreen_Line;

    using RequestType = std::variant<DrawRectRequestType, DrawLineRequestType>;
    using RequestList = TArray<RequestType>;

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

    UFUNCTION(BlueprintCallable)
    void
    Request_DrawLine_OnScreen(
        const FCk_Request_DebugDrawOnScreen_Line& InRequest);

private:
    auto
    DoOnPostRenderHUD(
        AHUD* InHUD,
        UCanvas* InCanvas) -> void;

    static auto
    DoHandleRequest(
        AHUD* InHUD,
        UCanvas* InCanvas,
        const FCk_Request_DebugDrawOnScreen_Rect& InRequest) -> void;

    static auto
    DoHandleRequest(
        AHUD* InHUD,
        UCanvas* InCanvas,
        const FCk_Request_DebugDrawOnScreen_Line& InRequest) -> void;

private:
    RequestList _PendingDrawRequests;

    FDelegateHandle _PostRenderHUD_DelegateHandle;
};

// --------------------------------------------------------------------------------------------------------------------
