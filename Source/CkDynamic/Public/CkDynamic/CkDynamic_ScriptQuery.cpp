#include "CkDynamic/CkDynamic_ScriptQuery.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_FragmentSchema.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_dynamic_script_query
{
    auto
        DoAddSlot(
            FCk_ScriptProcessorQuery& InQuery,
            const UScriptStruct* InType,
            ECk_ScriptQueryAccess InAccess)
        -> void
    {
        const auto TypeIsValid = ck::IsValid(InType);
        CK_ENSURE_IF_NOT(TypeIsValid,
            TEXT("Invalid fragment type passed to script processor query"))
        { InQuery._AdmissionFailed = true; return; }

        const auto AcceptsSlots = NOT InQuery._NoEntities;
        CK_ENSURE_IF_NOT(AcceptsSlots,
            TEXT("Cannot add fragment [{}] to a script processor query after NoEntities()"), InType)
        { InQuery._AdmissionFailed = true; return; }

        const auto Schema = ck::dynamic::Validate_FragmentSchema(InType);
        CK_ENSURE_IF_NOT(Schema.IsSafe,
            TEXT("Unsafe Dynamic Fragment schema [{}] cannot be registered in a script query; [{}]: {}"),
            InType, Schema.FailurePath, Schema.FailureReason)
        { InQuery._AdmissionFailed = true; return; }

        const auto AlreadyPresent = InQuery._Slots.ContainsByPredicate(
            [&](const FCk_ScriptQuerySlot& InSlot) { return InSlot._StructType == InType; });

        const auto SlotIsUnique = NOT AlreadyPresent;
        CK_ENSURE_IF_NOT(SlotIsUnique,
            TEXT("Fragment [{}] declared more than once in a script processor query. Keeping the first slot."), InType)
        { InQuery._AdmissionFailed = true; return; }

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
    ck_dynamic_script_query::DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::ReadWrite);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    ReadOnly(
        FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType)
    -> void
{
    ck_dynamic_script_query::DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::ReadOnly);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    Require(
        FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType)
    -> void
{
    ck_dynamic_script_query::DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::Require);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    Exclude(
        FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType)
    -> void
{
    ck_dynamic_script_query::DoAddSlot(InQuery, InType, ECk_ScriptQueryAccess::Exclude);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ScriptProcessorQuery_Mixin_UE::
    NoEntities(
        FCk_ScriptProcessorQuery& InQuery)
    -> void
{
    const auto HasNoSlots = InQuery._Slots.IsEmpty();
    CK_ENSURE_IF_NOT(HasNoSlots,
        TEXT("NoEntities() requires an empty slot list, but the query already has [{}] slot(s)"), InQuery._Slots.Num())
    { InQuery._AdmissionFailed = true; return; }

    InQuery._NoEntities = true;
}
