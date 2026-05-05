#pragma once

#include "CoreMinimal.h"
#include "CkRegistry_Handle.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// FCk_RegistryHandle — generational reference to an entt::basic_registry slot.
//
// SlotIndex == INDEX_NONE represents an "unset" handle. Generation == 0 is reserved as a
// never-allocated sentinel; every successful Allocate produces a Generation >= 1.
//
// Trivially copyable. Stored by value inside FCk_Handle. Resolution to a real
// entt::basic_registry* is done via ck::registry_table::Resolve.
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_RegistryHandle
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Ck|Registry")
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Ck|Registry")
    uint32 Generation = 0;

public:
    auto operator==(const FCk_RegistryHandle& InOther) const -> bool
    {
        return SlotIndex == InOther.SlotIndex && Generation == InOther.Generation;
    }

    auto operator!=(const FCk_RegistryHandle& InOther) const -> bool
    {
        return NOT (*this == InOther);
    }

    auto IsSet() const -> bool { return SlotIndex != INDEX_NONE; }

    static auto Unset() -> FCk_RegistryHandle { return FCk_RegistryHandle{}; }
};

template<>
struct TStructOpsTypeTraits<FCk_RegistryHandle> : public TStructOpsTypeTraitsBase2<FCk_RegistryHandle>
{
    enum { WithIdenticalViaEquality = true };
};
