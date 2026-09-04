#include "CkDynamic/CkDynamic_ScriptQueryBatch.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_FragmentSchema.h"
#include "CkDynamic/CkDynamic_Log.h"
#include "CkDynamic/CkDynamic_Sentinel.h"

#include <HAL/CriticalSection.h>
#include <Misc/ScopeLock.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptBindString.h>
#include <AngelscriptBinds.h>
#include <AngelscriptManager.h>
#include <ClassGenerator/ASStruct.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_script_query_batch
{
#if WITH_ANGELSCRIPT_CK
    auto
        ReturnFailedWildcardAccess(
            const UScriptStruct* InType)
        -> FScriptStructWildcard&
    {
        FAngelscriptManager::Throw("Script query batch access rejected invalid or stale input");
        static auto InvalidNoType = FScriptStructWildcard{};
        return ck::IsValid(InType)
            ? *(FScriptStructWildcard*)ck::dynamic::Get_InvalidSentinel_FragmentData(InType).GetMutableMemory()
            : InvalidNoType;
    }
#endif

    class FLiveStateResolver
    {
    public:
        auto Open(FCk_ScriptQueryBatchState& InState) -> uint64
        {
            const auto Lock = FScopeLock{&_Mutex};

            const auto StateIsClosed = NOT _LiveGenerations.Contains(&InState);
            CK_ENSURE_IF_NOT(StateIsClosed,
                TEXT("Script query batch state was opened twice without being closed"))
            { return 0; }

            const auto Generation = AllocateGeneration();
            _LiveGenerations.Add(&InState, Generation);

            // Arm the same-thread fast path: ForEachBatch runs synchronously on this thread, so
            // for the whole open window this (state, generation) pair proves liveness by
            // comparison alone — see Resolve.
            _ThreadOpenState      = &InState;
            _ThreadOpenGeneration = Generation;
            return Generation;
        }

        auto Close(FCk_ScriptQueryBatchState& InState, uint64 InGeneration) -> void
        {
            if (_ThreadOpenState == &InState)
            {
                _ThreadOpenState      = nullptr;
                _ThreadOpenGeneration = 0;
            }

            const auto Lock = FScopeLock{&_Mutex};
            const auto* LiveGeneration = _LiveGenerations.Find(&InState);

            const auto GenerationMatches = LiveGeneration != nullptr && *LiveGeneration == InGeneration;
            CK_ENSURE_IF_NOT(GenerationMatches,
                TEXT("Script query batch state closed with stale generation [{}]"), InGeneration)
            {
                // Independent of the ensure above on purpose: ensures compile out.
                ck::dynamic::Warning(
                    TEXT("Script query batch state closed with stale generation [{}]"), InGeneration);

                // Fail closed: leaving the older entry registered would let its batch resolve freed state.
                _LiveGenerations.Remove(&InState);
                return;
            }

            _LiveGenerations.Remove(&InState);
        }

        auto Resolve(const FCk_ScriptQueryBatch& InBatch) -> FCk_ScriptQueryBatchState*
        {
            if (InBatch._State == nullptr || InBatch._Generation == 0)
            { return nullptr; }

            // Fast path: the accessor is resolving the batch its own ForEachBatch call is
            // iterating (the only shape generated drivers produce). Pure comparison against the
            // thread-local open slot — no lock, and no dereference of an unproven pointer, so the
            // stale-stash protection below is untouched. With ~40 dispatching processors × N
            // entities × (2 + F) accessor calls per frame, this is thousands of game-thread lock
            // acquisitions removed. A nested or foreign open merely displaces the slot and the
            // displaced batch's accessors fall back to the locked registry: slower, never wrong.
            if (InBatch._State == _ThreadOpenState && InBatch._Generation == _ThreadOpenGeneration)
            { return InBatch._State; }

            const auto Lock = FScopeLock{&_Mutex};
            const auto* LiveGeneration = _LiveGenerations.Find(InBatch._State);
            if (LiveGeneration == nullptr || *LiveGeneration != InBatch._Generation)
            { return nullptr; }

            // ForEachBatch is synchronous, so the host cannot close this entry until the script call containing
            // the accessor returns: the lock protects registration, the call boundary protects the state.
            return InBatch._State;
        }

    private:
        auto AllocateGeneration() -> uint64
        {
            const auto Result = _NextGeneration++;
            if (_NextGeneration == 0)
            { _NextGeneration = 1; }
            return Result;
        }

    private:
        FCriticalSection                         _Mutex;
        TMap<FCk_ScriptQueryBatchState*, uint64> _LiveGenerations;
        uint64                                   _NextGeneration = 1;

        // The one batch state opened by THIS thread's in-flight DoDispatchBatch, if any. Written
        // only under Open/Close on the owning thread; read lock-free by Resolve on the same
        // thread. Never dereferenced — only compared.
        static thread_local FCk_ScriptQueryBatchState* _ThreadOpenState;
        static thread_local uint64                     _ThreadOpenGeneration;
    };

    thread_local FCk_ScriptQueryBatchState* FLiveStateResolver::_ThreadOpenState      = nullptr;
    thread_local uint64                     FLiveStateResolver::_ThreadOpenGeneration = 0;

    auto
        GetLiveStateResolver()
        -> FLiveStateResolver&
    {
        // Intentionally never freed: a stale script value may outlive world/module-owned processor state, so the
        // resolver must not participate in world teardown or static destruction ordering.
        static auto* Resolver = new FLiveStateResolver{};
        return *Resolver;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::dynamic::
    Open_ScriptQueryBatchState(
        FCk_ScriptQueryBatchState& InState)
    -> uint64
{
    const auto Generation = ck_dynamic_script_query_batch::GetLiveStateResolver().Open(InState);
    InState._Generation = Generation;
    return Generation;
}

auto
    ck::dynamic::
    Close_ScriptQueryBatchState(
        FCk_ScriptQueryBatchState& InState,
        uint64 InGeneration)
    -> void
{
    ck_dynamic_script_query_batch::GetLiveStateResolver().Close(InState, InGeneration);
    InState._Generation = 0;
}

auto
    ck::dynamic::
    Resolve_ScriptQueryBatchState(
        const FCk_ScriptQueryBatch& InBatch)
    -> FCk_ScriptQueryBatchState*
{
    return ck_dynamic_script_query_batch::GetLiveStateResolver().Resolve(InBatch);
}

auto
    ck::dynamic::
    Validate_ScriptQuerySlots(
        const FCk_ScriptProcessorQuery& InQuery,
        const TCHAR* InContext)
    -> bool
{
    for (const auto& Slot : InQuery._Slots)
    {
        const auto* Type = Slot._StructType.Get();
        const auto TypeIsValid = ck::IsValid(Type);
        CK_ENSURE_IF_NOT(TypeIsValid,
            TEXT("Script query [{}] contains an invalid fragment type; rejecting the complete query"), InContext)
        { return false; }

        const auto Schema = ck::dynamic::Validate_FragmentSchema(Type);
        CK_ENSURE_IF_NOT(Schema.IsSafe,
            TEXT("Script query [{}] contains unsafe Dynamic Fragment schema [{}]; [{}]: {}. "
                 "Rejecting the complete query."),
            InContext, Type, Schema.FailurePath, Schema.FailureReason)
        { return false; }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptQueryBatch_Mixin_UE::
    Num(
        const FCk_ScriptQueryBatch& InBatch)
    -> int32
{
    // Num stays quiet where Get/GetHandle ensure: it runs once per loop iteration and would spam.
    const auto* State = ck::dynamic::Resolve_ScriptQueryBatchState(InBatch);
    if (State == nullptr)
    { return 0; }

    return State->_Entities.Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptQueryBatch_Mixin_UE::
    GetHandle(
        const FCk_ScriptQueryBatch& InBatch,
        int32 InIndex)
    -> FCk_Handle
{
    auto* State = ck::dynamic::Resolve_ScriptQueryBatchState(InBatch);
    const auto StateIsLive = State != nullptr;
    CK_ENSURE_IF_NOT(StateIsLive,
        TEXT("Script query batch used past its ForEachBatch call (stale generation). Do not stash the batch."))
    { return FCk_Handle{}; }

    const auto IndexIsValid = State->_Entities.IsValidIndex(InIndex);
    CK_ENSURE_IF_NOT(IndexIsValid,
        TEXT("Script query batch GetHandle index [{}] out of range [0, {})"), InIndex, State->_Entities.Num())
    { return FCk_Handle{}; }

    return State->_AnyHandle.Get_ValidHandle(State->_Entities[InIndex]);
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

auto
    UCk_ScriptQueryBatch_Mixin_UE::
    Get(
        const FCk_ScriptQueryBatch& InBatch,
        int32 InIndex,
        const UScriptStruct* InType)
    -> FScriptStructWildcard&
{
    auto* State = ck::dynamic::Resolve_ScriptQueryBatchState(InBatch);
    const auto StateIsLive = State != nullptr;
    CK_ENSURE_IF_NOT(StateIsLive,
        TEXT("Script query batch used past its ForEachBatch call (stale generation). Do not stash the batch."))
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto TypeIsValid = ck::IsValid(InType);
    CK_ENSURE_IF_NOT(TypeIsValid,
        TEXT("Script query batch Get called with an invalid fragment type"))
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto IndexIsValid = State->_Entities.IsValidIndex(InIndex);
    CK_ENSURE_IF_NOT(IndexIsValid,
        TEXT("Script query batch Get index [{}] out of range [0, {}) for fragment [{}]"),
        InIndex, State->_Entities.Num(), InType)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto* Slot = State->_Slots.FindByPredicate(
        [&](const FCk_ScriptQueryBatchState::FSlot& InSlot)
        { return InSlot._Type == InType && InSlot._Access == ECk_ScriptQueryAccess::ReadWrite; });

    const auto SlotIsMutable = Slot != nullptr && Slot->_Storage != nullptr;
    CK_ENSURE_IF_NOT(SlotIsMutable,
        TEXT("Fragment [{}] is not a mutable ReadWrite slot in this processor's query"), InType)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    // Kept in shipping: catches a fragment removed mid-batch by a Request_Remove inside ForEachBatch.
    const auto Entity = State->_Entities[InIndex];
    const auto FragmentExists = Slot->_Storage->contains(Entity);
    CK_ENSURE_IF_NOT(FragmentExists,
        TEXT("Entity no longer carries fragment [{}] (removed mid-batch)"), InType)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    auto& Fragment = Slot->_Storage->get(Entity);
    return *(FScriptStructWildcard*)Fragment.Get_StructData().GetMemory();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptQueryBatch_Mixin_UE::
    GetReadOnly(
        const FCk_ScriptQueryBatch& InBatch,
        int32 InIndex,
        const UScriptStruct* InType)
    -> const FScriptStructWildcard&
{
    auto* State = ck::dynamic::Resolve_ScriptQueryBatchState(InBatch);
    const auto StateIsLive = State != nullptr;
    CK_ENSURE_IF_NOT(StateIsLive,
        TEXT("Script query batch used past its ForEachBatch call (stale generation). Do not stash the batch."))
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto TypeIsValid = ck::IsValid(InType);
    CK_ENSURE_IF_NOT(TypeIsValid,
        TEXT("Script query batch GetReadOnly called with an invalid fragment type"))
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto IndexIsValid = State->_Entities.IsValidIndex(InIndex);
    CK_ENSURE_IF_NOT(IndexIsValid,
        TEXT("Script query batch GetReadOnly index [{}] out of range [0, {}) for fragment [{}]"),
        InIndex, State->_Entities.Num(), InType)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto* Slot = State->_Slots.FindByPredicate(
        [&](const FCk_ScriptQueryBatchState::FSlot& InSlot)
        { return InSlot._Type == InType && InSlot._Access == ECk_ScriptQueryAccess::ReadOnly; });

    const auto SlotIsReadable = Slot != nullptr && Slot->_Storage != nullptr;
    CK_ENSURE_IF_NOT(SlotIsReadable,
        TEXT("Fragment [{}] is not a ReadOnly slot in this processor's query"), InType)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto Entity = State->_Entities[InIndex];
    const auto FragmentExists = Slot->_Storage->contains(Entity);
    CK_ENSURE_IF_NOT(FragmentExists,
        TEXT("Entity no longer carries fragment [{}] (removed mid-batch)"), InType)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto& Fragment = Slot->_Storage->get(Entity);
    return *(const FScriptStructWildcard*)Fragment.Get_StructData().GetMemory();
}

// --------------------------------------------------------------------------------------------------------------------

AS_FORCE_LINK const FAngelscriptBinds::FBind Bind_CkScriptQueryBatch(static_cast<int32>(FAngelscriptBinds::EOrder::Late), []
{
    auto ExistingClass = FAngelscriptBinds::ExistingClass("FCk_ScriptQueryBatch");

    ExistingClass.Method(
        "FScriptStructWildcard& Get(int InIndex, const UScriptStruct InType) const",
        [](const FCk_ScriptQueryBatch& Self, int32 InIndex, const UScriptStruct* InType) -> FScriptStructWildcard&
        {
            return UCk_ScriptQueryBatch_Mixin_UE::Get(Self, InIndex, InType);
        });
    FAngelscriptBinds::SetPreviousBindArgumentDeterminesOutputType(1);

    ExistingClass.Method(
        "const FScriptStructWildcard& GetReadOnly(int InIndex, const UScriptStruct InType) const",
        [](const FCk_ScriptQueryBatch& Self, int32 InIndex, const UScriptStruct* InType) -> const FScriptStructWildcard&
        {
            return UCk_ScriptQueryBatch_Mixin_UE::GetReadOnly(Self, InIndex, InType);
        });
    FAngelscriptBinds::SetPreviousBindArgumentDeterminesOutputType(1);
});

#endif
