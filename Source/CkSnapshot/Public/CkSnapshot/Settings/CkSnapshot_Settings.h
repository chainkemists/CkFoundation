#pragma once

#include <Engine/DeveloperSettings.h>

#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot_Settings.generated.h"

UENUM(BlueprintType)
enum class ECk_Snapshot_OrphanSweepGate : uint8
{
    Duration,
    NamedSignal,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Snapshot_OrphanSweepGate);

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="CkSnapshot"))
class CKSNAPSHOT_API UCk_Snapshot_Settings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Snapshot_Settings);

private:
    UPROPERTY(EditAnywhere, Config, Category="OrphanSweep")
    ECk_Snapshot_OrphanSweepGate _OrphanSweepGate = ECk_Snapshot_OrphanSweepGate::Duration;

    UPROPERTY(EditAnywhere, Config, Category="OrphanSweep",
              meta=(EditCondition="_OrphanSweepGate == ECk_Snapshot_OrphanSweepGate::Duration", ClampMin="0.0"))
    float _OrphanSweepGate_DurationSeconds = 2.0f;

    UPROPERTY(EditAnywhere, Config, Category="OrphanSweep",
              meta=(EditCondition="_OrphanSweepGate == ECk_Snapshot_OrphanSweepGate::NamedSignal"))
    FName _OrphanSweepGate_SignalName;

    UPROPERTY(EditAnywhere, Config, Category="Strictness")
    bool _RefuseLoadOnUnresolvedTypes = false;

    UPROPERTY(EditAnywhere, Config, Category="Strictness")
    bool _RefuseLoadsBelowBuildHash = false;

public:
    CK_PROPERTY_GET(_OrphanSweepGate);
    CK_PROPERTY_GET(_OrphanSweepGate_DurationSeconds);
    CK_PROPERTY_GET(_OrphanSweepGate_SignalName);
    CK_PROPERTY_GET(_RefuseLoadOnUnresolvedTypes);
    CK_PROPERTY_GET(_RefuseLoadsBelowBuildHash);
};
