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
            {}
            if (NOT StateIsClosed)
            { return 0; }

            const auto Generation = AllocateGeneration();
            _LiveGenerations.Add(&InState, Generation);
            return Generation;
        }

        auto Close(FCk_ScriptQueryBatchState& InState, uint64 InGeneration) -> void
        {
            const auto Lock = FScopeLock{&_Mutex};
            const auto* LiveGeneration = _LiveGenerations.Find(&InState);

            const auto GenerationMatches = LiveGeneration != nullptr && *LiveGeneration == InGeneration;
            CK_ENSURE_IF_NOT(GenerationMatches,
                TEXT("Script query batch state closed with stale generation [{}]"), InGeneration)
            {}
            if (NOT GenerationMatches)
            {
                // This warning is intentionally independent of the ensure above. Ensures can compile out or be
                // locally ignored, but a stale close is still useful operational evidence for a lifetime violation.
                ck::dynamic::Warning(
                    TEXT("Script query batch state closed with stale generation [{}]"), InGeneration);

                // A rejected close must still fail closed. Leaving an older entry registered would let its batch
                // resolve this host address after the state is destroyed.
                _LiveGenerations.Remove(&InState);
                return;
            }

            _LiveGenerations.Remove(&InState);
        }

        auto Resolve(const FCk_ScriptQueryBatch& InBatch) -> FCk_ScriptQueryBatchState*
        {
            if (InBatch._State == nullptr || InBatch._Generation == 0)
            { return nullptr; }

            const auto Lock = FScopeLock{&_Mutex};
            const auto* LiveGeneration = _LiveGenerations.Find(InBatch._State);
            if (LiveGeneration == nullptr || *LiveGeneration != InBatch._Generation)
            { return nullptr; }

            // ForEachBatch is synchronous: the host cannot close this entry until the script call containing this
            // accessor returns. The resolver lock protects registration, while the call boundary protects the state.
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
    };

    auto
        GetLiveStateResolver()
        -> FLiveStateResolver&
    {
        // Intentional process-lifetime allocation. A stale script value may survive world/module-owned processor
        // state, so the resolver itself must not participate in world teardown or static destruction ordering.
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
        {}
        if (NOT TypeIsValid)
        { return false; }

        const auto Schema = ck::dynamic::Validate_FragmentSchema(Type);
        CK_ENSURE_IF_NOT(Schema.IsSafe,
            TEXT("Script query [{}] contains unsafe Dynamic Fragment schema [{}]; [{}]: {}. "
                 "Rejecting the complete query."),
            InContext, Type, Schema.FailurePath, Schema.FailureReason)
        {}
        if (NOT Schema.IsSafe)
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
    // A stashed (stale-generation) or default-constructed batch reports empty so a stray driver loop over it is a
    // no-op rather than a crash. Explicit accessors (Get/GetHandle) ensure loudly; Num stays quiet — it is called
    // once per loop iteration and would otherwise spam.
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
    {}
    if (NOT StateIsLive)
    { return FCk_Handle{}; }

    const auto IndexIsValid = State->_Entities.IsValidIndex(InIndex);
    CK_ENSURE_IF_NOT(IndexIsValid,
        TEXT("Script query batch GetHandle index [{}] out of range [0, {})"), InIndex, State->_Entities.Num())
    {}
    if (NOT IndexIsValid)
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
    // (1) Generation guard — stashed batch.
    auto* State = ck::dynamic::Resolve_ScriptQueryBatchState(InBatch);
    const auto StateIsLive = State != nullptr;
    CK_ENSURE_IF_NOT(StateIsLive,
        TEXT("Script query batch used past its ForEachBatch call (stale generation). Do not stash the batch."))
    {}
    if (NOT StateIsLive)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    // (2) Null type — no sentinel key available.
    const auto TypeIsValid = ck::IsValid(InType);
    CK_ENSURE_IF_NOT(TypeIsValid,
        TEXT("Script query batch Get called with an invalid fragment type"))
    {}
    if (NOT TypeIsValid)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    // (3) Index bounds.
    const auto IndexIsValid = State->_Entities.IsValidIndex(InIndex);
    CK_ENSURE_IF_NOT(IndexIsValid,
        TEXT("Script query batch Get index [{}] out of range [0, {}) for fragment [{}]"),
        InIndex, State->_Entities.Num(), InType)
    {}
    if (NOT IndexIsValid)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    // (4) Slot resolution — mutable access is available only to an explicitly ReadWrite slot.
    const auto* Slot = State->_Slots.FindByPredicate(
        [&](const FCk_ScriptQueryBatchState::FSlot& InSlot)
        { return InSlot._Type == InType && InSlot._Access == ECk_ScriptQueryAccess::ReadWrite; });

    const auto SlotIsMutable = Slot != nullptr && Slot->_Storage != nullptr;
    CK_ENSURE_IF_NOT(SlotIsMutable,
        TEXT("Fragment [{}] is not a mutable ReadWrite slot in this processor's query"), InType)
    {}
    if (NOT SlotIsMutable)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    // (5) contains-probe — kept in shipping; catches a fragment removed mid-batch (Request_Remove).
    const auto Entity = State->_Entities[InIndex];
    const auto FragmentExists = Slot->_Storage->contains(Entity);
    CK_ENSURE_IF_NOT(FragmentExists,
        TEXT("Entity no longer carries fragment [{}] (removed mid-batch)"), InType)
    {}
    if (NOT FragmentExists)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    // (6) Live registry storage read, returned through the wildcard (same reinterpret idiom as Get_Fragment).
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
    {}
    if (NOT StateIsLive)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto TypeIsValid = ck::IsValid(InType);
    CK_ENSURE_IF_NOT(TypeIsValid,
        TEXT("Script query batch GetReadOnly called with an invalid fragment type"))
    {}
    if (NOT TypeIsValid)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto IndexIsValid = State->_Entities.IsValidIndex(InIndex);
    CK_ENSURE_IF_NOT(IndexIsValid,
        TEXT("Script query batch GetReadOnly index [{}] out of range [0, {}) for fragment [{}]"),
        InIndex, State->_Entities.Num(), InType)
    {}
    if (NOT IndexIsValid)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto* Slot = State->_Slots.FindByPredicate(
        [&](const FCk_ScriptQueryBatchState::FSlot& InSlot)
        { return InSlot._Type == InType && InSlot._Access == ECk_ScriptQueryAccess::ReadOnly; });

    const auto SlotIsReadable = Slot != nullptr && Slot->_Storage != nullptr;
    CK_ENSURE_IF_NOT(SlotIsReadable,
        TEXT("Fragment [{}] is not a ReadOnly slot in this processor's query"), InType)
    {}
    if (NOT SlotIsReadable)
    { return ck_dynamic_script_query_batch::ReturnFailedWildcardAccess(InType); }

    const auto Entity = State->_Entities[InIndex];
    const auto FragmentExists = Slot->_Storage->contains(Entity);
    CK_ENSURE_IF_NOT(FragmentExists,
        TEXT("Entity no longer carries fragment [{}] (removed mid-batch)"), InType)
    {}
    if (NOT FragmentExists)
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
