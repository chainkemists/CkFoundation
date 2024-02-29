#include "CkEntity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Entity_UE::
    IsEqual(
        const FCk_Entity& InEntityA,
        const FCk_Entity& InEntityB)
    -> bool
{
    return InEntityA == InEntityB;
}

auto
    UCk_Utils_Entity_UE::
    IsNotEqual(
        const FCk_Entity& InEntityA,
        const FCk_Entity& InEntityB)
    -> bool
{
    return InEntityA != InEntityB;
}

auto
    UCk_Utils_Entity_UE::
    Conv_EntityToText(
        const FCk_Entity& InEntity)
    -> FText
{
    return FText::FromString(Conv_EntityToString(InEntity));
}

auto
    UCk_Utils_Entity_UE::
    Conv_EntityToString(
        const FCk_Entity& InEntity)
    -> FString
{
    return ck::Format_UE(TEXT("{}"), InEntity);
}

auto
    UCk_Utils_Entity_UE::
    Get_TombstoneEntity()
    -> const FCk_Entity&
{
    return FCk_Entity::Tombstone();
}

auto
    UCk_Utils_Entity_UE::
    Break_Entity(
        const FCk_Entity& InEntity,
        int32& OutEntityID,
        int32& OutEntityNumber,
        int32& OutEntityVersion)
    -> void
{
    OutEntityID = static_cast<int32>(InEntity.Get_ID());
    OutEntityNumber = InEntity.Get_EntityNumber();
    OutEntityVersion = InEntity.Get_VersionNumber();
}

// --------------------------------------------------------------------------------------------------------------------
