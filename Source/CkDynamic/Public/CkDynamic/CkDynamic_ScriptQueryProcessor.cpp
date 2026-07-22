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
    // Duplicated from the anonymous namespace of CkDynamic_Utils.cpp; kept local to avoid touching the
    // hot Get_Fragment TU. Consolidate into the shared internal header (CkDynamic_Sentinel.h).
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

        // The generated driver is a SUBCLASS of the dev class, so instantiating it yields one object carrying both
        // the author's surface (BeginPlay/EndPlay/ForEachEntity) and the driver's Configure/ForEachBatch overrides.
        // Direct mode (no driver class): the dev class overrides ForEachBatch itself.
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

        // Resolve the query once. On a generated driver, Configure adds the inferred slots then calls Super::Configure
        // when the dev class overrides it; a direct-mode dev fills the whole query here.
        _Instance->Configure(_Query);

        const auto AdmissionSucceeded = NOT _Query._AdmissionFailed;
        CK_ENSURE_IF_NOT(AdmissionSucceeded,
            TEXT("Script processor [{}] query admission failed. Disabling it instead of dispatching a partial query."),
            InDevClass)
        {}
        if (NOT AdmissionSucceeded)
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
            // An all-Exclude query has no pool to drive the join (DoResolveAndJoin would return false every tick),
            // so the processor would construct fine and then silently never run. Fail loudly here instead.
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
        // EndPlay only mirrors a BeginPlay that actually ran (BeginPlay is the last thing the ctor does, and only when
        // not disabled).
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
        if (_Disabled)
        { return; }

        FScopeCycleCounter TickStatCounter{_TickStatId};

        if (ck::Is_NOT_Valid(_Instance.Get()))
        { return; }

        // Dead-transient-handle skip (mirrors FProcessor_ScriptHosted): during registry teardown the scheduler keeps
        // ticking for a few frames after the transient entity is destroyed. Every registry access below would ensure
        // on the dead handle — skip the dispatch entirely, matching how C++ processors see an empty view.
        if (ck::Is_NOT_Valid(_Instance->Get_Handle()))
        { return; }

        if (_Query._NoEntities)
        {
            // No join: run every tick with an empty batch.
            _BatchState._Slots.Reset();
            _BatchState._Entities.Reset();
            _BatchState._AnyHandle = _Instance->Get_Handle();
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
        -> int32
    {
        Tick(TimeType::ZeroSecond());

        // The batch dispatch runs script whose side effects we can't inspect — "unknown" tells the
        // scheduler to conservatively treat the pump as having done work (see FTickable_Concept::
        // Pump). Mirrors FProcessor_ScriptHosted; wiring the join count is a possible refinement.
        constexpr auto VisitedCountUnknown = -1;
        return VisitedCountUnknown;
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

        // Drive the smallest pool; contains()-probe every slot (Exclude inverts), honoring the destroy filter.
        for (const auto Entity : static_cast<const entt::sparse_set&>(*DriveStorage))
        {
            if (NOT RegistryView.IsValid(Entity))
            { continue; }

            auto EntityHandle = AnyHandle.Get_ValidHandle(Entity);
            if (NOT ck_dynamic_script_query_processor::PassesDestroyFilter(EntityHandle, ECk_DestroyFilter::IgnorePendingKill))
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

        // A structurally valid join that matched nothing would still cost a VM call to
        // dispatch a zero-iteration batch — skip it. NoEntities is the sanctioned route for
        // processors that want to run without a join.
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
            // Always remove the resolver entry before this dispatch scope exits. A script exception or other
            // abnormal return must not leave the host address registered after its access window has closed.
            ck::dynamic::Close_ScriptQueryBatchState(_BatchState, Generation);
        };

        _Instance->ForEachBatch(Batch, InDeltaT);
    }
}
