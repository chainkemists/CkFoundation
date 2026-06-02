#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "CkSnapshot/SaveGame/CkSnapshot_Header.h"
#include "CkSnapshot/Snapshot/CkSnapshot_LoadReport.h"
#include "CkSnapshot/Subsystem/CkSnapshot_Delegates.h"

#include <Subsystems/GameInstanceSubsystem.h>

#include "CkSnapshot_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, BlueprintType, DisplayName="CkSubsystem_Snapshot")
class CKSNAPSHOT_API UCk_Snapshot_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Snapshot_Subsystem_UE);

public:
    // Saves the current ECS world to InSlotName. Server/authority only. Pumps the world to quiescence
    // (via UCk_EcsWorld_Subsystem_UE::Request_PumpToQuiescence) before capturing so the snapshot reflects
    // a settled world, then writes a UCk_Snapshot_SaveGame to the slot. Broadcasts OnPreSave / OnSaveComplete
    // and fires InDelegate with the result.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Save",
              meta = (AutoCreateRefTerm = "InDelegate"))
    void
    Request_Save(
        FName InSlotName,
        const FCk_Delegate_OnSaveComplete& InDelegate);

    // Loads the snapshot in InSlotName back into the current ECS world. Server/authority only. Validates the
    // format version, then restores via the manifest-driven loader. Broadcasts OnPreLoad / OnLoadComplete and
    // fires InDelegate with the load report.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Request Load",
              meta = (AutoCreateRefTerm = "InDelegate"))
    void
    Request_Load(
        FName InSlotName,
        const FCk_Delegate_OnLoadComplete& InDelegate);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Has Save Slot")
    bool
    Get_HasSaveSlot(
        FName InSlotName) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Get Save Slot Header")
    FCk_Snapshot_Header
    Get_SaveSlotHeader(
        FName InSlotName) const;

    UFUNCTION(BlueprintPure,
              Category = "Ck|Snapshot",
              DisplayName = "[Ck][Snapshot] Try Resolve Save Key")
    bool
    TryResolve_SaveKey(
        FGuid InKey,
        FCk_Handle& OutHandle) const;

public:
    // SaveKey resolver -- maps a stable FGuid (stored on the FFragment_SaveKey of a saved entity) to the
    // live FCk_Handle. Populated during load so post-load consumers can re-acquire entities by key.
    auto Publish_SaveKey(FGuid InKey, FCk_Handle InHandle) -> void;
    auto Consume_SaveKey(FGuid InKey) -> void;

#if WITH_AUTOMATION_TESTS
public:
    // Phase 8 test hook: the pump-pass count Request_Save observed on its last invocation.
    auto TestOnly_LastPumpCount() const -> int32 { return _LastPumpCount; }
#endif

private:
    auto DoGet_SnapshotSource() const -> FCk_Handle;

private:
    UPROPERTY(Transient)
    TMap<FGuid, FCk_Handle> _SaveKeyResolverMap;

    bool _SnapshotInProgress = false;
    int32 _LastPumpCount = 0;
};

// --------------------------------------------------------------------------------------------------------------------
