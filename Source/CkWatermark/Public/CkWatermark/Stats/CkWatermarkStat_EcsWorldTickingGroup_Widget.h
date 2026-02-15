#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkWatermark/Stats/CkWatermarkStat_Base_Widget.h"

#include <GameplayTagContainer.h>

#include "CkWatermarkStat_EcsWorldTickingGroup_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Shows the average cycle time (ms) of one specific ECS World ticking group.
// Set _TickingGroup to one of the EcsWorldTickingGroup.* gameplay tags per widget instance.

UCLASS(meta = (DisplayName = "Ck Watermark Stat - ECS Ticking Group",
               Category    = "CkFoundation|Watermark Stats"))
class CKWATERMARK_API UCkWatermarkStat_EcsWorldTickingGroup_UWidget_UE : public UCkWatermarkStat_Base_UWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCkWatermarkStat_EcsWorldTickingGroup_UWidget_UE);

    auto NativeGetStatName()  const -> FText override;
    auto NativeGetStatValue() const -> FText override;

private:
    // Which ticking group this widget instance should display.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watermark|ECS",
              meta = (AllowPrivateAccess = true, Categories = "EcsWorldTickingGroup"))
    FGameplayTag _TickingGroup;

    // Whether to read client-side or server-side stat data.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Watermark|ECS",
              meta = (AllowPrivateAccess = true))
    ECk_ClientServer _StatSource = ECk_ClientServer::Client;

public:
    CK_PROPERTY_GET(_TickingGroup);
    CK_PROPERTY_GET(_StatSource);
};

// --------------------------------------------------------------------------------------------------------------------
