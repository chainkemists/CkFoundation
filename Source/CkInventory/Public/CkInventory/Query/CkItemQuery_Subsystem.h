#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkInventory/Query/CkItemQuery_Fragment_Data.h"

#include "Engine/StreamableManager.h"
#include "Subsystems/EngineSubsystem.h"

#include "CkItemQuery_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_InventoryItem_Definition;
struct FAssetData;

// --------------------------------------------------------------------------------------------------------------------

// Process-global cache of every UCk_InventoryItem_Definition, built once (async)
// from the AssetRegistry. Item definitions are world-agnostic data assets, so an
// engine subsystem builds the index a single time and shares it across worlds —
// replacing the previous per-call AssetRegistry scan + synchronous load.
//
// In the editor the index is invalidated when definition assets are added/removed/
// renamed (AssetRegistry events) or when a designer edits a definition's traits
// (FCoreUObjectDelegates::OnObjectPropertyChanged). Invalidation is lazy: the cache
// is dropped and rebuilt on the next Request_BuildIndex (i.e. the next query).
UCLASS(NotBlueprintable, NotBlueprintType, DisplayName = "CkSubsystem_ItemQuery")
class CKINVENTORY_API UCk_ItemQuery_Subsystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ItemQuery_Subsystem_UE);

public:
    auto
    Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

    auto
    Deinitialize() -> void override;

public:
    static auto
    Get() -> UCk_ItemQuery_Subsystem_UE*;

public:
    auto
    Get_IsIndexReady() const -> bool;

    // Idempotent — kicks the async build if it hasn't started and isn't ready.
    auto
    Request_BuildIndex() -> void;

    // Filters the cached index. Caller must ensure the index is ready — returns
    // empty when not yet built.
    auto
    Query_Definitions(
        const FCk_ItemQuery_Filter& InFilter) const -> TArray<TObjectPtr<UCk_InventoryItem_Definition>>;

private:
    auto
    DoGatherDefinitionPaths() const -> TArray<FSoftObjectPath>;

    auto
    DoOnIndexLoaded() -> void;

    // Drops the cached index so the next Request_BuildIndex rebuilds from scratch.
    auto
    DoInvalidateIndex() -> void;

#if WITH_EDITOR
    auto
    DoOnAssetChanged(
        const FAssetData& InAssetData) -> void;

    auto
    DoOnAssetRenamed(
        const FAssetData& InAssetData,
        const FString& InOldObjectPath) -> void;

    auto
    DoOnObjectPropertyChanged(
        UObject* InObject,
        struct FPropertyChangedEvent& InPropertyChangedEvent) -> void;
#endif

private:
    UPROPERTY(Transient)
    TArray<TObjectPtr<UCk_InventoryItem_Definition>> _AllDefinitions;

    bool _IsIndexReady = false;
    bool _BuildInFlight = false;

    TSharedPtr<FStreamableHandle> _LoadHandle;

#if WITH_EDITOR
    FDelegateHandle _OnAssetAddedHandle;
    FDelegateHandle _OnAssetRemovedHandle;
    FDelegateHandle _OnAssetUpdatedHandle;
    FDelegateHandle _OnAssetRenamedHandle;
    FDelegateHandle _OnObjectPropertyChangedHandle;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
