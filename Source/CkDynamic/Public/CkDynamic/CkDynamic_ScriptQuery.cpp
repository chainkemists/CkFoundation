#include "CkDynamic/CkDynamic_ScriptQuery.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace
{
    auto
        DoAddSlot(
            FCk_ScriptProcessorQuery& InQuery,
            const UScriptStruct* InType,
            ECk_ScriptQueryAccess InAccess)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InType),
            TEXT("Invalid fragment type passed to script processor query"))
        { return; }

        const auto AlreadyPresent = InQuery._Slots.ContainsByPredicate(
            [&](const FCk_ScriptQuerySlot& InSlot) { return InSlot._StructType == InType; });

        CK_ENSURE_IF_NOT(NOT AlreadyPresent,
            TEXT("Fragment [{}] declared more than once in a script processor query. Keeping the first slot."), InType)
        { return; }

        auto Slot = FCk_ScriptQuerySlot{};
        Slot._StructType = InType;
        Slot._Access = InAccess;
        InQuery._Slots.Add(Slot);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    ReadWrite(
        FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType)
    -> void
{
    DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::ReadWrite);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    ReadOnly(
        FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType)
    -> void
{
    DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::ReadOnly);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    Require(
        FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType)
    -> void
{
    DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::Require);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    Exclude(
        FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType)
    -> void
{
    DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::Exclude);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    NoEntities(
        FCk_ScriptProcessorQuery& InQuery)
    -> void
{
    CK_ENSURE_IF_NOT(InQuery._Slots.IsEmpty(),
        TEXT("NoEntities() requires an empty slot list, but the query already has [{}] slot(s)"), InQuery._Slots.Num())
    { return; }

    InQuery._NoEntities = true;
}
