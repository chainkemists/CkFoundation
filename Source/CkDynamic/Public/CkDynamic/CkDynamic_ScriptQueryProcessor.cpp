#include "CkDynamic/CkDynamic_ScriptQueryProcessor.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Utils.h"        // Get_StorageId
#include "CkDynamic/CkDynamic_Fragment.h"     // ck::FFragment_DynamicFragment_Data
#include "CkDynamic/CkDynamic_Fragment_Data.h"// ECk_DestroyFilter

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Processor/CkProcessor_Script.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <Misc/ScopeExit.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_script_query_processor
{
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

        // The driver is a SUBCLASS of the dev class, so ONE instance carries both surfaces.
        auto* InstanceClass = ck::IsValid(InDriverClass) ? InDriverClass : InDevClass;
        auto* Instance = NewObject<UCk_Processor_Script_Base_UE>(GetTransientPackage(), InstanceClass);
        _Instance = TStrongObjectPtr<UCk_Processor_Script_Base_UE>{Instance};

        CK_ENSURE_IF_NOT(ck::IsValid(_Instance.Get()),
            TEXT("FProcessor_ScriptQueryHosted failed to construct an instance for [{}]"), InstanceClass)
        { _Disabled = true; return; }

        // Stat row keyed by the DEV class name — that is the name authors know and RunAfter references use.
        _TickStatId = CK_CREATE_DYNAMIC_STAT_ID(STATGROUP_CkProcessors,
            ck::Format_UE(TEXT("script::{}"), InDevClass->GetName()));

        const auto TransientHandle = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(InRegistry);
        _Instance->Set_Handle(TransientHandle);

        _Instance->Configure(_Query);

        const auto AdmissionSucceeded = NOT _Query._AdmissionFailed;
        CK_ENSURE_IF_NOT(AdmissionSucceeded,
            TEXT("Script processor [{}] query admission failed. Disabling it instead of dispatching a partial query."),
            InDevClass)
        { _Disabled = true; return; }

        const auto QueryContext = InDevClass->GetPathName();
        if (NOT ck::dynamic::Validate_ScriptQuerySlots(_Query, *QueryContext))
        { _Disabled = true; return; }

        const auto QueryIsValid = (NOT _Query._Slots.IsEmpty()) || _Query._NoEntities;
        CK_ENSURE_IF_NOT(QueryIsValid,
            TEXT("Script processor [{}] declared an empty query and did not call NoEntities(). Disabling it."), InDevClass)
        { _Disabled = true; return; }

        if (NOT _Query._NoEntities)
        {
            const auto HasNonExcludeSlot = _Query._Slots.ContainsByPredicate(
                [](const FCk_ScriptQuerySlot& InSlot) { return InSlot._Access != ECk_ScriptQueryAccess::Exclude; });

            CK_ENSURE_IF_NOT(HasNonExcludeSlot,
                TEXT("Script processor [{}] declared only Exclude slots — there is no pool to drive the join. "
                     "Add a ReadOnly/ReadWrite/Require slot or call NoEntities(). Disabling it."), InDevClass)
            { _Disabled = true; return; }
        }

        _Instance->BeginPlay();
    }

    // ----------------------------------------------------------------------------------------------------------------

    FProcessor_ScriptQueryHosted::
        ~FProcessor_ScriptQueryHosted()
    {
        // EndPlay only mirrors a BeginPlay that actually ran — the ctor calls it last, and only when not disabled.
        if (NOT _Disabled && ck::IsValid(_Instance.Get()))
        {
            _Instance->EndPlay();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        _LastTickVisitedCount = 0;

        if (_Disabled)
        { return; }

        FScopeCycleCounter TickStatCounter{_TickStatId};

        if (ck::Is_NOT_Valid(_Instance.Get()))
        { return; }

        // During registry teardown the scheduler keeps ticking for a few frames after the transient entity is
        // destroyed; every registry access below would ensure on the dead handle. C++ processors see an empty view.
        if (ck::Is_NOT_Valid(_Instance->Get_Handle()))
        { return; }

        if (_Query._NoEntities)
        {
            _BatchState._Slots.Reset();
            _BatchState._Entities.Reset();
            _BatchState._AnyHandle = _Instance->Get_Handle();

            constexpr auto VisitedCountUnknown = -1;
            _LastTickVisitedCount = VisitedCountUnknown;

            DoDispatchBatch(InDeltaT);
            return;
        }

        if (NOT DoResolveAndJoin())
        { return; }   // empty-join early-out — no VM call

        _LastTickVisitedCount = _BatchState._Entities.Num();

        DoDispatchBatch(InDeltaT);
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        Pump()
        -> int32
    {
        Tick(TimeType::ZeroSecond());
        return _LastTickVisitedCount;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoResolveAndJoin()
        -> bool
    {
        const auto QueryContext = _Instance->GetClass()->GetPathName();
        if (NOT ck::dynamic::Validate_ScriptQuerySlots(_Query, *QueryContext))
        {
            _Disabled = true;
            return false;
        }

        _BatchState._Slots.Reset();
        _BatchState._Entities.Reset();

        auto AnyHandle = _Instance->Get_Handle();
        _BatchState._AnyHandle = AnyHandle;
        auto RegistryView = AnyHandle.Get_RegistryView();   // by value: a lightweight view over the shared registry

        entt::storage<ck::FFragment_DynamicFragment_Data>* DriveStorage = nullptr;
        auto SmallestCount = MAX_int32;

        for (const auto& Slot : _Query._Slots)
        {
            const auto* Type = Slot._StructType.Get();

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

        for (const auto Entity : static_cast<const entt::sparse_set&>(*DriveStorage))
        {
            if (NOT RegistryView.IsValid(Entity))
            { continue; }

            auto EntityHandle = AnyHandle.Get_ValidHandle(Entity);
            if (NOT ck_dynamic_script_query_processor::PassesDestroyFilter(EntityHandle, ECk_DestroyFilter::IgnorePendingKill))
            { continue; }

            // This join is hand-rolled over the storages rather than built through a view, so it is the one
            // admission path MakeProcessorView cannot cover — and a script query's Exclude slots resolve only
            // dynamic-fragment storages, so they cannot name this C++ tag either.
            if (EntityHandle.Has<ck::FTag_Hydration_Quarantine>())
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

        // NoEntities is the sanctioned route for a processor that wants to run without a join.
        return _BatchState._Entities.Num() > 0;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ScriptQueryHosted::
        DoDispatchBatch(
            TimeType InDeltaT)
        -> void
    {
        const auto Generation = ck::dynamic::Open_ScriptQueryBatchState(_BatchState);
        if (Generation == 0)
        { return; }

        auto Batch = FCk_ScriptQueryBatch{};
        Batch._State = &_BatchState;
        Batch._Generation = Generation;

        ON_SCOPE_EXIT
        {
            // A script exception must not leave the host address registered past its access window.
            ck::dynamic::Close_ScriptQueryBatchState(_BatchState, Generation);
        };

        _Instance->ForEachBatch(Batch, InDeltaT);
    }
}
