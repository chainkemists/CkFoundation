#include "CkGroundNav/Streaming/CkGroundNav_StreamPartitionLifecycle.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <HAL/CriticalSection.h>
#include <Engine/World.h>
#include <Misc/ScopeLock.h>

namespace ck::groundnav::stream_partitions
{
    namespace stream_partition_lifecycle_private
    {
        struct FPartitionState
        {
            uint64 _Generation = 0;
            bool _IsActive = false;
            bool _IsEnabled = false;
            world_fields::FCk_GroundNav_StreamSourceHandle _Source;
        };

        struct FManifestOwnerState
        {
            FCk_Handle _OwnerEntity;
            uint64 _Instance = 0;
            uint64 _DefaultFingerprint = 0;
            TMap<FGameplayTag, uint64> _VariantFingerprints;
            FCk_GroundNav_DataLayerSelector _DataLayerSelector;
            FName _SourceLevelPackage;
            FName _CookKey;
        };

        struct FWorldState
        {
            TMap<FCk_GroundNav_StreamPartitionKey, FPartitionState> _Partitions;
            TMap<FCk_GroundNav_VolumeId, FManifestOwnerState> _ManifestOwners;
            TMap<FCk_GroundNav_VolumeId, uint64> _NextOwnerInstances;
        };

        auto Get_Lock() -> FCriticalSection&
        {
            static FCriticalSection Lock;
            return Lock;
        }

        auto Get_WorldStates() -> TMap<TWeakObjectPtr<UWorld>, FWorldState>&
        {
            static TMap<TWeakObjectPtr<UWorld>, FWorldState> States;
            return States;
        }

        auto DoBind_CleanupHookOnce() -> void
        {
            static auto CleanupHookIsBound = false;
            if (CleanupHookIsBound) { return; }
            CleanupHookIsBound = true;
            FWorldDelegates::OnWorldCleanup.AddLambda(
                [](UWorld* InWorld, bool /*InSessionEnded*/, bool /*InCleanupResources*/) -> void
                {
                    auto Lock = FScopeLock{&Get_Lock()};
                    Get_WorldStates().Remove(TWeakObjectPtr<UWorld>{InWorld});
                });
        }

        auto Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus InStatus) -> FCk_GroundNav_StreamPartitionLifecycleResult
        {
            auto Result = FCk_GroundNav_StreamPartitionLifecycleResult{};
            Result._Status = InStatus;
            return Result;
        }

        auto Make_RegistryResult(
            const world_fields::FCk_GroundNav_StreamRegistryResult& InRegistry) -> FCk_GroundNav_StreamPartitionLifecycleResult
        {
            auto Result = Make_Result(
                InRegistry.Get_Succeeded() ? ECk_GroundNav_StreamPartitionLifecycleStatus::Published
                                            : ECk_GroundNav_StreamPartitionLifecycleStatus::RegistryRefused);
            Result._Registry = InRegistry;
            return Result;
        }

        auto Get_IsNewerGeneration(const uint64 InGeneration, const FPartitionState* InState) -> bool
        {
            return InGeneration != 0 && (InState == nullptr || InGeneration > InState->_Generation);
        }

        auto Get_HasValidLifecycleInput(
            UWorld* InWorld, const FCk_GroundNav_StreamPartitionKey& InKey, const uint64 InGeneration,
            FCk_GroundNav_StreamPartitionLifecycleResult& OutFailure) -> bool
        {
            const auto WorldIsValid = ck::IsValid(InWorld);
            CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream partition lifecycle requires a valid world"))
            {}
            if (NOT WorldIsValid)
            {
                OutFailure = Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidWorld);
                return false;
            }

            const auto KeyIsValid = InKey.Get_IsValid();
            CK_ENSURE_IF_NOT(KeyIsValid, TEXT("GroundNav stream partition lifecycle requires positive volume and partition ids"))
            {}
            if (NOT KeyIsValid)
            {
                OutFailure = Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidKey);
                return false;
            }

            const auto GenerationIsValid = InGeneration != 0;
            CK_ENSURE_IF_NOT(GenerationIsValid, TEXT("GroundNav stream partition lifecycle requires a positive generation"))
            {}
            if (NOT GenerationIsValid)
            {
                OutFailure = Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidGeneration);
                return false;
            }

            return true;
        }

        auto Get_HasCurrentOwnerInstance(
            const FWorldState* InWorldState,
            const FCk_GroundNav_StreamPartitionKey& InKey,
            const uint64 InOwnerInstance) -> bool
        {
            const auto* Owner = InWorldState == nullptr ? nullptr : InWorldState->_ManifestOwners.Find(InKey._VolumeId);
            return InOwnerInstance != 0 && Owner != nullptr && Owner->_Instance == InOwnerInstance;
        }
    }

    auto GetTypeHash(const FCk_GroundNav_StreamPartitionKey& InKey) -> uint32
    {
        return HashCombine(ck::groundnav::GetTypeHash(InKey._VolumeId), ::GetTypeHash(InKey._PartitionId));
    }

    auto Load(
        UWorld* InWorld, const FCk_GroundNav_StreamPartitionLoad& InLoad) -> FCk_GroundNav_StreamPartitionLifecycleResult
    {
        using namespace stream_partition_lifecycle_private;

        auto Failure = FCk_GroundNav_StreamPartitionLifecycleResult{};
        if (NOT Get_HasValidLifecycleInput(InWorld, InLoad._Key, InLoad._Generation, Failure))
        { return Failure; }

        const auto TransitionsArePresent = NOT InLoad._Transitions.IsEmpty();
        CK_ENSURE_IF_NOT(TransitionsArePresent, TEXT("GroundNav stream partition load requires manifest transitions"))
        {}
        if (NOT TransitionsArePresent)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidTransitions); }

        DoBind_CleanupHookOnce();
        auto Lock = FScopeLock{&Get_Lock()};
        const auto WorldKey = TWeakObjectPtr<UWorld>{InWorld};
        auto* WorldState = Get_WorldStates().Find(WorldKey);
        if (NOT Get_HasCurrentOwnerInstance(WorldState, InLoad._Key, InLoad._OwnerInstance))
        { return Make_Result(InLoad._OwnerInstance == 0 ? ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidOwnerInstance : ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }
        const auto* Existing = WorldState->_Partitions.Find(InLoad._Key);
        if (NOT Get_IsNewerGeneration(InLoad._Generation, Existing))
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }
        if (Existing != nullptr && Existing->_IsActive)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::DuplicateActivePartition); }

        const auto Registry = world_fields::Load_StreamSource(InWorld, InLoad._Key._VolumeId, InLoad._Transitions);
        auto Result = Make_RegistryResult(Registry);
        if (NOT Registry.Get_Succeeded())
        { return Result; }

        WorldState->_Partitions.Add(InLoad._Key, FPartitionState{InLoad._Generation, true, true, Registry._Source});
        return Result;
    }

    auto Register_ManifestOwner(
        UWorld* InWorld, const FCk_Handle& InOwnerEntity, const FCk_GroundNav_VolumeId InVolumeId,
        const FCk_GroundNav_StreamFieldBundle& InInitialBundle, const uint64 InDefaultFingerprint,
        const TMap<FGameplayTag, uint64>& InVariantFingerprints,
        const FCk_GroundNav_DataLayerSelector& InDataLayerSelector,
        const FName InSourceLevelPackage, const FName InCookKey) -> FCk_GroundNav_StreamPartitionLifecycleResult
    {
        using namespace stream_partition_lifecycle_private;

        const auto WorldIsValid = ck::IsValid(InWorld);
        CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav manifest owner registration requires a valid world")) {}
        if (NOT WorldIsValid) { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidWorld); }
        const auto OwnerIsValid = ck::IsValid(InOwnerEntity);
        CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("GroundNav manifest owner registration requires a valid owner")) {}
        if (NOT OwnerIsValid) { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidOwner); }
        const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
        CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav manifest owner registration requires a positive volume id")) {}
        if (NOT VolumeIdIsValid) { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidKey); }

        DoBind_CleanupHookOnce();
        auto Lock = FScopeLock{&Get_Lock()};
        const auto WorldKey = TWeakObjectPtr<UWorld>{InWorld};
        const auto* ExistingWorldState = Get_WorldStates().Find(WorldKey);
        if (ExistingWorldState != nullptr && ExistingWorldState->_ManifestOwners.Contains(InVolumeId))
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::RegistryRefused); }

        const auto Registry = world_fields::Register_StreamOwner(
            InWorld, InOwnerEntity, InVolumeId, InInitialBundle, false);
        auto Result = Make_RegistryResult(Registry);
        if (NOT Registry.Get_Succeeded()) { return Result; }

        auto& WorldState = Get_WorldStates().FindOrAdd(WorldKey);
        auto& NextInstance = WorldState._NextOwnerInstances.FindOrAdd(InVolumeId);
        ++NextInstance;
        WorldState._ManifestOwners.Add(InVolumeId, FManifestOwnerState{
            InOwnerEntity, NextInstance, InDefaultFingerprint, InVariantFingerprints, InDataLayerSelector,
            InSourceLevelPackage, InCookKey});
        Result._OwnerInstance = NextInstance;
        return Result;
    }

    auto TryGet_ManifestOwner(
        UWorld* InWorld, const FCk_GroundNav_VolumeId InVolumeId) -> TOptional<FCk_GroundNav_ManifestOwnerSnapshot>
    {
        using namespace stream_partition_lifecycle_private;

        if (ck::Is_NOT_Valid(InWorld) || NOT InVolumeId.Get_IsStreamingValid())
        { return {}; }

        auto Lock = FScopeLock{&Get_Lock()};
        const auto* WorldState = Get_WorldStates().Find(TWeakObjectPtr<UWorld>{InWorld});
        const auto* Owner = WorldState == nullptr ? nullptr : WorldState->_ManifestOwners.Find(InVolumeId);
        if (Owner == nullptr || NOT ck::IsValid(Owner->_OwnerEntity) || Owner->_Instance == 0)
        { return {}; }

        auto Snapshot = FCk_GroundNav_ManifestOwnerSnapshot{};
        Snapshot._OwnerEntity = Owner->_OwnerEntity;
        Snapshot._OwnerInstance = Owner->_Instance;
        Snapshot._DefaultFingerprint = Owner->_DefaultFingerprint;
        Snapshot._VariantFingerprints = Owner->_VariantFingerprints;
        Snapshot._DataLayerSelector = Owner->_DataLayerSelector;
        Snapshot._SourceLevelPackage = Owner->_SourceLevelPackage;
        Snapshot._CookKey = Owner->_CookKey;
        return Snapshot;
    }

    auto Deactivate(
        UWorld* InWorld, const FCk_GroundNav_StreamPartitionKey& InKey,
        const uint64 InOwnerInstance, const uint64 InGeneration) -> FCk_GroundNav_StreamPartitionLifecycleResult
    {
        using namespace stream_partition_lifecycle_private;

        auto Failure = FCk_GroundNav_StreamPartitionLifecycleResult{};
        if (NOT Get_HasValidLifecycleInput(InWorld, InKey, InGeneration, Failure))
        { return Failure; }

        auto Lock = FScopeLock{&Get_Lock()};
        auto* WorldState = Get_WorldStates().Find(TWeakObjectPtr<UWorld>{InWorld});
        if (NOT Get_HasCurrentOwnerInstance(WorldState, InKey, InOwnerInstance))
        { return Make_Result(InOwnerInstance == 0 ? ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidOwnerInstance : ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }
        auto* State = WorldState->_Partitions.Find(InKey);
        if (State == nullptr || NOT State->_IsActive)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::PartitionNotActive); }
        if (NOT Get_IsNewerGeneration(InGeneration, State))
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }
        if (NOT State->_IsEnabled)
        {
            State->_Generation = InGeneration;
            return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::NoChange);
        }

        const auto Registry = world_fields::Disable_StreamSource(InWorld, InKey._VolumeId, State->_Source);
        auto Result = Make_RegistryResult(Registry);
        if (NOT Registry.Get_Succeeded())
        { return Result; }

        State->_Generation = InGeneration;
        State->_IsEnabled = false;
        return Result;
    }

    auto Reactivate(
        UWorld* InWorld, const FCk_GroundNav_StreamPartitionKey& InKey,
        const uint64 InOwnerInstance, const uint64 InGeneration) -> FCk_GroundNav_StreamPartitionLifecycleResult
    {
        using namespace stream_partition_lifecycle_private;

        auto Failure = FCk_GroundNav_StreamPartitionLifecycleResult{};
        if (NOT Get_HasValidLifecycleInput(InWorld, InKey, InGeneration, Failure))
        { return Failure; }

        auto Lock = FScopeLock{&Get_Lock()};
        auto* WorldState = Get_WorldStates().Find(TWeakObjectPtr<UWorld>{InWorld});
        if (NOT Get_HasCurrentOwnerInstance(WorldState, InKey, InOwnerInstance))
        { return Make_Result(InOwnerInstance == 0 ? ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidOwnerInstance : ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }
        auto* State = WorldState->_Partitions.Find(InKey);
        if (State == nullptr || NOT State->_IsActive)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::PartitionNotActive); }
        if (NOT Get_IsNewerGeneration(InGeneration, State))
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }
        if (State->_IsEnabled)
        {
            State->_Generation = InGeneration;
            return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::NoChange);
        }

        const auto Registry = world_fields::Enable_StreamSource(InWorld, InKey._VolumeId, State->_Source);
        auto Result = Make_RegistryResult(Registry);
        if (NOT Registry.Get_Succeeded())
        { return Result; }

        State->_Generation = InGeneration;
        State->_IsEnabled = true;
        return Result;
    }

    auto Unload(
        UWorld* InWorld, const FCk_GroundNav_StreamPartitionKey& InKey,
        const uint64 InOwnerInstance, const uint64 InGeneration) -> FCk_GroundNav_StreamPartitionLifecycleResult
    {
        using namespace stream_partition_lifecycle_private;

        auto Failure = FCk_GroundNav_StreamPartitionLifecycleResult{};
        if (NOT Get_HasValidLifecycleInput(InWorld, InKey, InGeneration, Failure))
        { return Failure; }

        auto Lock = FScopeLock{&Get_Lock()};
        auto* WorldState = Get_WorldStates().Find(TWeakObjectPtr<UWorld>{InWorld});
        if (NOT Get_HasCurrentOwnerInstance(WorldState, InKey, InOwnerInstance))
        { return Make_Result(InOwnerInstance == 0 ? ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidOwnerInstance : ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }
        auto* State = WorldState->_Partitions.Find(InKey);
        if (State == nullptr || NOT State->_IsActive)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::PartitionNotActive); }
        if (NOT Get_IsNewerGeneration(InGeneration, State))
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::StaleEvent); }

        const auto Registry = world_fields::Unload_StreamSource(InWorld, InKey._VolumeId, State->_Source);
        auto Result = Make_RegistryResult(Registry);
        if (NOT Registry.Get_Succeeded())
        { return Result; }

        State->_Generation = InGeneration;
        State->_IsActive = false;
        State->_IsEnabled = false;
        State->_Source = {};
        return Result;
    }

    auto Purge_OwnerPartitions(
        UWorld* InWorld, const FCk_Handle& InOwnerEntity,
        const FCk_GroundNav_VolumeId InVolumeId) -> FCk_GroundNav_StreamPartitionLifecycleResult
    {
        using namespace stream_partition_lifecycle_private;

        const auto WorldIsValid = ck::IsValid(InWorld);
        CK_ENSURE_IF_NOT(WorldIsValid, TEXT("GroundNav stream partition purge requires a valid world"))
        {}
        if (NOT WorldIsValid)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidWorld); }
        const auto OwnerIsValid = ck::IsValid(InOwnerEntity);
        CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("GroundNav stream partition purge requires a valid owner"))
        {}
        if (NOT OwnerIsValid)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidOwner); }
        const auto VolumeIdIsValid = InVolumeId.Get_IsStreamingValid();
        CK_ENSURE_IF_NOT(VolumeIdIsValid, TEXT("GroundNav stream partition purge requires a positive volume id"))
        {}
        if (NOT VolumeIdIsValid)
        { return Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::InvalidKey); }

        auto Lock = FScopeLock{&Get_Lock()};
        auto* WorldState = Get_WorldStates().Find(TWeakObjectPtr<UWorld>{InWorld});
        const auto* ManifestOwner = WorldState == nullptr ? nullptr : WorldState->_ManifestOwners.Find(InVolumeId);
        if (ManifestOwner == nullptr || ManifestOwner->_OwnerEntity != InOwnerEntity)
        {
            auto Result = Make_Result(ECk_GroundNav_StreamPartitionLifecycleStatus::RegistryRefused);
            Result._Registry._Status = world_fields::ECk_GroundNav_StreamRegistryStatus::OwnerNotRegistered;
            return Result;
        }
        const auto Registry = world_fields::Unregister_StreamOwner(InWorld, InOwnerEntity, InVolumeId);
        auto Result = Make_RegistryResult(Registry);
        if (NOT Registry.Get_Succeeded())
        { return Result; }

        auto PartitionKeysToRemove = TArray<FCk_GroundNav_StreamPartitionKey>{};
        for (const auto& Partition : WorldState->_Partitions)
        {
            if (Partition.Key._VolumeId == InVolumeId)
            { PartitionKeysToRemove.Add(Partition.Key); }
        }
        for (const auto& PartitionKey : PartitionKeysToRemove)
        { WorldState->_Partitions.Remove(PartitionKey); }
        WorldState->_ManifestOwners.Remove(InVolumeId);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
