#include "CkArchetype_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/Archetype/CkArchetype_Registry.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Archetype_UE::
    Request_Register(
        const FCk_ArchetypeDescriptor& InDescriptor)
    -> void
{
    ck::archetype_registry::Register(InDescriptor);
}

auto
    UCk_Utils_Archetype_UE::
    Request_Register_FromDefinition(
        const UCk_ArchetypeDefinition* InDefinition)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InDefinition),
        TEXT("Cannot register Archetype from an INVALID definition asset"))
    { return; }

    CK_ENSURE_IF_NOT(InDefinition->IsValidDefinition(),
        TEXT("Cannot register Archetype from definition [{}] — Name is not set"), InDefinition->GetName())
    { return; }

    ck::archetype_registry::Register(InDefinition->Get_Descriptor());
}

auto
    UCk_Utils_Archetype_UE::
    Get_Matches(
        const FCk_Handle& InHandle,
        FName InArchetypeName)
    -> bool
{
    return ck::archetype_registry::Get_Matches(InHandle, InArchetypeName);
}

auto
    UCk_Utils_Archetype_UE::
    TryGet_BestMatchName(
        const FCk_Handle& InHandle)
    -> FName
{
    return ck::archetype_registry::TryGet_BestMatchName(InHandle);
}

auto
    UCk_Utils_Archetype_UE::
    Get_AllArchetypeNames()
    -> TArray<FName>
{
    auto Result = TArray<FName>{};
    for (const auto& Descriptor : ck::archetype_registry::Get_All())
    { Result.Add(Descriptor.Get_Name()); }

    return Result;
}
