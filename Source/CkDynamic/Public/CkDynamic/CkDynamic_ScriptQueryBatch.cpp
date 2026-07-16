#include "CkDynamic/CkDynamic_ScriptQueryBatch.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Sentinel.h"

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptBindString.h>
#include <AngelscriptBinds.h>
#include <ClassGenerator/ASStruct.h>
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_script_query_batch
{
    auto
        Is_LiveBatch(
            const FCk_ScriptQueryBatch& InBatch)
        -> bool
    {
        return InBatch._State != nullptr && InBatch._Generation == InBatch._State->_Generation;
    }
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
    if (NOT ck_dynamic_script_query_batch::Is_LiveBatch(InBatch))
    { return 0; }

    return InBatch._State->_Entities.Num();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptQueryBatch_Mixin_UE::
    GetHandle(
        const FCk_ScriptQueryBatch& InBatch,
        int32 InIndex)
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck_dynamic_script_query_batch::Is_LiveBatch(InBatch),
        TEXT("Script query batch used past its ForEachBatch call (stale generation). Do not stash the batch."))
    { return FCk_Handle{}; }

    auto& State = *InBatch._State;

    CK_ENSURE_IF_NOT(State._Entities.IsValidIndex(InIndex),
        TEXT("Script query batch GetHandle index [{}] out of range [0, {})"), InIndex, State._Entities.Num())
    { return FCk_Handle{}; }

    return State._AnyHandle.Get_ValidHandle(State._Entities[InIndex]);
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
    static auto InvalidNoType = FScriptStructWildcard{};

    // (1) Generation guard — stashed batch.
    CK_ENSURE_IF_NOT(ck_dynamic_script_query_batch::Is_LiveBatch(InBatch),
        TEXT("Script query batch used past its ForEachBatch call (stale generation). Do not stash the batch."))
    {
        return ck::IsValid(InType)
            ? *(FScriptStructWildcard*)ck::dynamic::Get_InvalidSentinel_FragmentData(InType).GetMutableMemory()
            : InvalidNoType;
    }

    // (2) Null type — no sentinel key available.
    CK_ENSURE_IF_NOT(ck::IsValid(InType),
        TEXT("Script query batch Get called with an invalid fragment type"))
    { return InvalidNoType; }

    auto& State = *InBatch._State;

    // (3) Index bounds.
    CK_ENSURE_IF_NOT(State._Entities.IsValidIndex(InIndex),
        TEXT("Script query batch Get index [{}] out of range [0, {}) for fragment [{}]"),
        InIndex, State._Entities.Num(), InType)
    { return *(FScriptStructWildcard*)ck::dynamic::Get_InvalidSentinel_FragmentData(InType).GetMutableMemory(); }

    // (4) Slot resolution — the fragment must be a declared, non-Exclude slot with resolved storage.
    const auto* Slot = State._Slots.FindByPredicate(
        [&](const FCk_ScriptQueryBatchState::FSlot& InSlot)
        { return InSlot._Type == InType && InSlot._Access != ECk_ScriptQueryAccess::Exclude; });

    CK_ENSURE_IF_NOT(Slot != nullptr && Slot->_Storage != nullptr,
        TEXT("Fragment [{}] is not a readable slot in this processor's query"), InType)
    { return *(FScriptStructWildcard*)ck::dynamic::Get_InvalidSentinel_FragmentData(InType).GetMutableMemory(); }

    // (5) contains-probe — kept in shipping; catches a fragment removed mid-batch (Request_Remove).
    const auto Entity = State._Entities[InIndex];
    CK_ENSURE_IF_NOT(Slot->_Storage->contains(Entity),
        TEXT("Entity no longer carries fragment [{}] (removed mid-batch)"), InType)
    { return *(FScriptStructWildcard*)ck::dynamic::Get_InvalidSentinel_FragmentData(InType).GetMutableMemory(); }

    // (6) Live registry storage read, returned through the wildcard (same reinterpret idiom as Get_Fragment).
    auto& Fragment = Slot->_Storage->get(Entity);
    return *(FScriptStructWildcard*)Fragment.Get_StructData().GetMemory();
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
});

#endif
