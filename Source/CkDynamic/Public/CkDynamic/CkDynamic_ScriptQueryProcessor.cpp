#include "CkDynamic/CkDynamic_ScriptQueryProcessor.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Utils.h"        // Get_StorageId
#include "CkDynamic/CkDynamic_Fragment.h"     // ck::FFragment_DynamicFragment_Data
#include "CkDynamic/CkDynamic_Fragment_Data.h"// ECk_DestroyFilter

#include "CkEcs/Processor/CkProcessor_Script.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    // Duplicated from the anonymous namespace of CkDynamic_Utils.cpp. Task 12 folds both copies into the shared
    // internal header (CkDynamic_Sentinel.h); kept local here to avoid touching the hot Get_Fragment TU now.
    auto
        PassesDestroyFilter(
            const FCk_Handle& InHandle,
            ECk_DestroyFilter InFilter)
        -> bool
    {
        switch (InFilter)
        {
            case ECk_DestroyFilter::None:
            {
                return true;
            }
            case ECk_DestroyFilter::IgnorePendingKill:
            {
                return NOT (UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::BeginDestroy) ||
                            UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Teardown) ||
                            UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Destroyed));
            }
            case ECk_DestroyFilter::Teardown:
            {
                return UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(InHandle, ECk_EntityLifetime_DestructionPhase::Teardown);
            }
            default:
            {
                CK_INVALID_ENUM(InFilter);
                return false;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_ScriptQueryHosted::
        FProcessor_ScriptQueryHosted(
            const RegistryType& InRegistry,
            UClass* InDevClass,
            UClass* InDriverClass)
        : _Registry(InRegistry)
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InDevClass),
            TEXT("FProcessor_ScriptQueryHosted constructed with a null dev UClass"))
        { _Disabled = true; return; }

        auto* DevInstance = NewObject<UCk_Processor_Script_Base_UE>(GetTransientPackage(), InDevClass);
        _DevInstance = TStrongObjectPtr<UCk_Processor_Script_Base_UE>{DevInstance};

        CK_ENSURE_IF_NOT(ck::IsValid(_DevInstance.Get()),
            TEXT("FProcessor_ScriptQueryHosted failed to construct a dev instance for [{}]"), InDevClass)
        { _Disabled = true; return; }

        const auto TransientHandle = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InRegistry);

        if (ck::IsValid(InDriverClass))
        {
            auto* DriverInstance = NewObject<UCk_Processor_Script_Base_UE>(GetTransientPackage(), InDriverClass);
            _BatchInstance = TStrongObjectPtr<UCk_Processor_Script_Base_UE>{DriverInstance};

            CK_ENSURE_IF_NOT(ck::IsValid(_BatchInstance.Get()),
                TEXT("FProcessor_ScriptQueryHosted failed to construct driver instance [{}]"), InDriverClass)
            { _Disabled = true; return; }

            _BatchInstance->Set_IterationTarget(_DevInstance.Get());
        }
        else
        {
            // Direct mode: the dev class overrides ForEachBatch itself; the two roles are the same object.
            _BatchInstance = _DevInstance;
        }

        _DevInstance->Set_Handle(TransientHandle);
        _BatchInstance->Set_Handle(TransientHandle);

        // Resolve the query once. On a generated driver, Configure adds the inferred slots then forwards to the dev's
        // Configure; a direct-mode dev fills the whole query here.
        _BatchInstance->Configure(_Query);

        const auto QueryIsValid = (NOT _Query._Slots.IsEmpty()) || _Query._NoEntities;
        CK_ENSURE_IF_NOT(QueryIsValid,
            TEXT("Script processor [{}] declared an empty query and did not call NoEntities(). Disabling it."), InDevClass)
        { _Disabled = true; return; }

        _DevInstance->BeginPlay();
    }

    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_ScriptQueryHosted::
        ~FProcessor_ScriptQueryHosted()
    {
        // EndPlay only mirrors a BeginPlay that actually ran (BeginPlay is the last thing the ctor does, and only when
        // not disabled).
        if (NOT _Disabled && ck::IsValid(_DevInstance.Get()))
        {
            _DevInstance->EndPlay();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        if (_Disabled)
        { return; }

        if (ck::Is_NOT_Valid(_DevInstance.Get()) || ck::Is_NOT_Valid(_BatchInstance.Get()))
        { return; }

        // Dead-transient-handle skip (mirrors FProcessor_ScriptHosted): during registry teardown the scheduler keeps
        // ticking for a few frames after the transient entity is destroyed. Every registry access below would ensure
        // on the dead handle — skip the dispatch entirely, matching how C++ processors see an empty view.
        if (ck::Is_NOT_Valid(_DevInstance->Get_Handle()))
        { return; }

        if (_Query._NoEntities)
        {
            // No join: run every tick with an empty batch.
            _BatchState._Slots.Reset();
            _BatchState._Entities.Reset();
            _BatchState._AnyHandle = _DevInstance->Get_Handle();
            DoDispatchBatch(InDeltaT);
            return;
        }

        if (NOT DoResolveAndJoin())
        { return; }   // empty-join early-out — no VM call

        DoDispatchBatch(InDeltaT);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        Pump()
        -> void
    {
        Tick(TimeType::ZeroSecond());
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoResolveAndJoin()
        -> bool
    {
        _BatchState._Slots.Reset();
        _BatchState._Entities.Reset();

        auto AnyHandle = _DevInstance->Get_Handle();
        _BatchState._AnyHandle = AnyHandle;
        auto RegistryView = AnyHandle.Get_RegistryView();   // by value: a lightweight view over the shared registry

        entt::storage<ck::FFragment_DynamicFragment_Data>* DriveStorage = nullptr;
        auto SmallestCount = MAX_int32;

        for (const auto& Slot : _Query._Slots)
        {
            const auto* Type = Slot._StructType.Get();
            if (ck::Is_NOT_Valid(Type))
            { continue; }

            const auto StorageId = UCk_Utils_DynamicFragment_UE::Get_StorageId(Type);
            auto& Storage = RegistryView.Storage<ck::FFragment_DynamicFragment_Data>(StorageId);

            auto StateSlot = FCk_ScriptQueryBatchState::FSlot{};
            StateSlot._Type = Type;
            StateSlot._Storage = &Storage;
            StateSlot._Access = Slot._Access;
            _BatchState._Slots.Add(StateSlot);

            if (Slot._Access == ECk_ScriptQueryAccess::Exclude)
            { continue; }   // exclude pools neither drive nor gate emptiness

            const auto Count = static_cast<int32>(Storage.size());
            if (Count == 0)
            { return false; }   // a required/read pool is empty -> the join is empty

            if (Count < SmallestCount)
            {
                SmallestCount = Count;
                DriveStorage = &Storage;
            }
        }

        if (DriveStorage == nullptr)
        { return false; }   // no non-exclude slots -> nothing to iterate

        // Drive the smallest pool; contains()-probe every slot (Exclude inverts), honoring the destroy filter.
        for (const auto Entity : static_cast<const entt::sparse_set&>(*DriveStorage))
        {
            if (NOT RegistryView.IsValid(Entity))
            { continue; }

            auto EntityHandle = AnyHandle.Get_ValidHandle(Entity);
            if (NOT PassesDestroyFilter(EntityHandle, ECk_DestroyFilter::IgnorePendingKill))
            { continue; }

            auto PassesJoin = true;
            for (const auto& StateSlot : _BatchState._Slots)
            {
                const auto Contains = StateSlot._Storage->contains(Entity);
                const auto IsExclude = StateSlot._Access == ECk_ScriptQueryAccess::Exclude;
                if (IsExclude ? Contains : (NOT Contains))
                { PassesJoin = false; break; }
            }

            if (NOT PassesJoin)
            { continue; }

            _BatchState._Entities.Add(Entity);
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoDispatchBatch(
            TimeType InDeltaT)
        -> void
    {
        auto Batch = FCk_ScriptQueryBatch{};
        Batch._State = &_BatchState;
        Batch._Generation = _BatchState._Generation;

        _BatchInstance->ForEachBatch(Batch, InDeltaT);

        // Invalidate any batch the script stashed past the call.
        ++_BatchState._Generation;
    }
}
