#pragma once

#include "CoreMinimal.h"
#include "CkRegistry_Handle.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// SlotIndex == INDEX_NONE is an "unset" handle; Generation == 0 is the never-allocated sentinel
// (every successful Allocate produces >= 1). Resolved via ck::registry_table::Resolve.
// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKECS_API FCk_RegistryHandle
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Ck|Registry")
    int32 SlotIndex = INDEX_NONE;

    // int32 (not uint32) because UHT/BP doesn't reflect uint32 — an opaque token, sign bit
    // irrelevant. The slot table's increment-on-alloc/free skips 0 on wrap.
    UPROPERTY(BlueprintReadOnly, Category = "Ck|Registry")
    int32 Generation = 0;

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
