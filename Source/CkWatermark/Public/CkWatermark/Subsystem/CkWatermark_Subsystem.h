#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkWatermark/CkWatermark_Panel_Widget.h"
#include "CkWatermark/CkWatermark_ActivityBar_Types.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkWatermark_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_Watermark")
class CKWATERMARK_API UCk_Watermark_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Watermark_Subsystem_UE);

public:
    auto Deinitialize() -> void override;

private:
    auto PlayerControllerChanged(APlayerController* InNewPlayerController) -> void override;

public:
    auto Request_UpdateWatermarkDisplayPolicy(ECk_Watermark_DisplayPolicy InDisplayPolicy) const -> void;
    auto ForceRebuildWidget() -> void;

    // ---- Activity Bar API --------------------------------------------------------
    // Game code calls these to push signal state changes into the watermark.
    // Signals appear as chips in the activity bar above the stat rows.

    UFUNCTION(BlueprintCallable, Category = "Watermark|Activity")
    void Request_ActivityActive(FName InActivityId, const FText& InDisplayLabel);

    UFUNCTION(BlueprintCallable, Category = "Watermark|Activity")
    void Request_ActivityInactive(FName InActivityId);

    // Accessors for the activity bar Slate widget.
    auto Get_ActivityStates()  const -> const TArray<FCkWatermarkActivityState>&;
    auto Get_ActivityVersion() const -> uint32;

    // ---- Static convenience helpers (single-node Blueprint call) -----------------
    // Resolve PlayerController → LocalPlayer → Subsystem internally.

    UFUNCTION(BlueprintCallable, Category = "Watermark|Activity",
              meta = (DefaultToSelf = "InPlayerController"))
    static void NotifyActivityActive(
        APlayerController* InPlayerController,
        FName              InActivityId,
        const FText&       InDisplayLabel);

    UFUNCTION(BlueprintCallable, Category = "Watermark|Activity",
              meta = (DefaultToSelf = "InPlayerController"))
    static void NotifyActivityInactive(
        APlayerController* InPlayerController,
        FName              InActivityId);

private:
    auto DoCreateAndSetWatermarkWidget(APlayerController* InPlayerController) -> void;
    auto DoLogStaticInfo(const APlayerController* InPlayerController) const -> void;
    auto DoSortActivityStates() -> void;
    auto DoTrimActivityHistory() -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCkWatermark_Panel_UWidget_UE> _WatermarkWidget;

    bool _StaticInfoLogged = false;

    // ---- Activity Bar state ------------------------------------------------------
    TArray<FCkWatermarkActivityState> _ActivityStates;
    uint32 _ActivityVersion = 0;

    // Monotonically increasing counters per activity Id for permanent numbering.
    TMap<FName, int32> _ActivitySequenceCounters;
};

// --------------------------------------------------------------------------------------------------------------------
