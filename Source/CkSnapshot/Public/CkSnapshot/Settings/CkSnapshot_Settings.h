#pragma once

#include <Engine/DeveloperSettings.h>

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkSnapshot_Settings.generated.h"

UENUM(BlueprintType)
enum class ECk_Snapshot_OrphanSweepGate : uint8
{
    Duration,
    NamedSignal,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Snapshot_OrphanSweepGate);

// Not a logging-verbosity knob: establishing whether a skipped entity carried a payload costs a full Produce
// sweep per entity, so each mode buys diagnostic reach with save-frame time.
UENUM(BlueprintType)
enum class ECk_Snapshot_CaptureAuditMode : uint8
{
    Disabled,  // no probe, no dropped-payload signal; the only mode a shipped save pays nothing for
    Summary,   // probe, then one aggregated Warning with counts and example entities
    Detailed,  // one Warning per entity with the dropped payload's full ExportText (~0.45ms each)
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Snapshot_CaptureAuditMode);

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

    // A project shipping an autosave wants Disabled here and Summary on its dev configuration.
    UPROPERTY(EditAnywhere, Config, Category="CaptureAudit")
    ECk_Snapshot_CaptureAuditMode _CaptureAuditMode = ECk_Snapshot_CaptureAuditMode::Summary;

    UPROPERTY(EditAnywhere, Config, Category="CaptureAudit",
              meta=(EditCondition="_CaptureAuditMode == ECk_Snapshot_CaptureAuditMode::Summary", ClampMin="0"))
    int32 _CaptureAudit_MaxExamples = 5;

    // Fork-join from the game thread: workers serialize the already-produced payload copies while the game
    // thread participates and blocks, so GC — which only ever starts from the game thread — cannot run
    // mid-serialize. Disable only to rule the parallel path out while diagnosing a save-side crash.
    UPROPERTY(EditAnywhere, Config, Category="Save")
    ECk_EnableDisable _ParallelPayloadSerialization = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_OrphanSweepGate);
    CK_PROPERTY_GET(_OrphanSweepGate_DurationSeconds);
    CK_PROPERTY_GET(_OrphanSweepGate_SignalName);
    CK_PROPERTY_GET(_RefuseLoadOnUnresolvedTypes);
    CK_PROPERTY_GET(_RefuseLoadsBelowBuildHash);
    CK_PROPERTY_GET(_CaptureAuditMode);
    CK_PROPERTY_GET(_CaptureAudit_MaxExamples);
    CK_PROPERTY_GET(_ParallelPayloadSerialization);

#if WITH_AUTOMATION_TESTS
public:
    // A test asserting audit behaviour must pin the mode: the project's own configuration would
    // otherwise decide whether the assertion is even reachable.
    auto TestOnly_Set_CaptureAuditMode(ECk_Snapshot_CaptureAuditMode InMode) -> void
    { _CaptureAuditMode = InMode; }

    // The parallel-vs-serial parity gate pins each mode explicitly rather than inheriting the project's.
    auto TestOnly_Set_ParallelPayloadSerialization(ECk_EnableDisable InMode) -> void
    { _ParallelPayloadSerialization = InMode; }
#endif
};
