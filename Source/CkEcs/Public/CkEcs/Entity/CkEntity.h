#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkThirdParty/entt-3.12.2/src/entt/entity/entity.hpp"

#include <CoreMinimal.h>

#include <CkEntity.generated.h>

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak="/Script/CkEcs.Ck_Utils_Entity_UE:Break_Entity"))
struct CKECS_API FCk_Entity
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Entity);

public:
    using IdType = entt::entity;
    using TombstoneType = entt::tombstone_t;
    using EntityNumberType = entt::entt_traits<IdType>::entity_type;
    using VersionNumberType = entt::entt_traits<IdType>::version_type;

public:
    FCk_Entity(IdType InID = entt::tombstone);
    FCk_Entity(TombstoneType);

    auto operator=(IdType InOther) noexcept -> ThisType&;
    auto operator=(TombstoneType InOther) noexcept -> ThisType&;

    auto operator==(ThisType InOther) const -> bool;
    auto operator<(ThisType InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATORS(ThisType);

public:
    auto
    Get_IsTombstone() const -> bool;

    auto
    Get_EntityNumber() const -> EntityNumberType;

    auto
    Get_VersionNumber() const -> VersionNumberType;

public:
    static auto
    Tombstone() -> const FCk_Entity&;

private:
    IdType _ID;

public:
    CK_PROPERTY_GET(_ID)

#if WITH_EDITORONLY_DATA
private:
    UPROPERTY()
    int32 _EntityID;

    UPROPERTY()
    int32 _EntityNumber;

    UPROPERTY()
    int32 _EntityVersion;
#endif
};

// --------------------------------------------------------------------------------------------------------------------

template<>
struct TStructOpsTypeTraits<FCk_Entity> : public TStructOpsTypeTraitsBase2<FCk_Entity>
{
    enum
    {
        WithIdenticalViaEquality = true,
    };
};

// --------------------------------------------------------------------------------------------------------------------

auto CKECS_API GetTypeHash(FCk_Entity InEntity) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

#if NOT WITH_EDITORONLY_DATA
static_assert
(
    sizeof(FCk_Entity) == sizeof(FCk_Entity::IdType),
    "Entity should be nothing more than an ID"
);
#endif

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Entity, [&]()
{
    if (InObj.Get_IsTombstone())
    { return ck::Format(TEXT("Tombstone")); }

    return ck::Format
    (
        TEXT("{}|{}({})"),
        static_cast<int32>(InObj.Get_EntityNumber()),
        static_cast<int32>(InObj.Get_VersionNumber()),
        static_cast<int32>(InObj.Get_ID())
    );
});

// --------------------------------------------------------------------------------------------------------------------

CK_DELETE_CUSTOM_IS_VALID(FCk_Entity);

// --------------------------------------------------------------------------------------------------------------------
