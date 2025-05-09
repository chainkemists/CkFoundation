#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>
#include <Net/Serialization/FastArraySerializer.h>

#include "CkGameplayTagStack.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_GameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_GameplayTagStack : public FFastArraySerializerItem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayTagStack);

public:
    friend FCk_GameplayTagStackContainer;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FGameplayTag _Tag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true,  UIMin = 0, ClampMin = 0))
    int32 _StackCount = 0;

public:
    CK_PROPERTY_GET(_Tag);
    CK_PROPERTY_GET(_StackCount);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GameplayTagStack, _Tag, _StackCount);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_GameplayTagStack, [](const FCk_GameplayTagStack& InObj)
{
    return ck::Format
    (
        TEXT("{}x{}"),
        InObj.Get_Tag(),
        InObj.Get_StackCount()
    );
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType,
    meta = (HasNativeMake = "/Script/CkCore.Ck_Utils_GameplayTagStack_UE:Make_GameplayTagStackContainer",
            HasNativeBreak = "/Script/CkCore.Ck_Utils_GameplayTagStack_UE:Break_GameplayTagStackContainer"))
struct FCk_GameplayTagStackContainer : public FFastArraySerializer
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GameplayTagStackContainer);

public:
    using GameplayTagStackType = FCk_GameplayTagStack;

public:
    auto
    AddStack(
        const FGameplayTag& InTag,
        int32 InStackCount) -> FCk_GameplayTagStackContainer&;

    auto
    AddStack(
        const FCk_GameplayTagStack& InStack) -> FCk_GameplayTagStackContainer&;

    auto
    RemoveStack(
        const FGameplayTag& InTag,
        int32 InStackCount) -> FCk_GameplayTagStackContainer&;

    auto
    RemoveStack(
        const FCk_GameplayTagStack& InStack) -> FCk_GameplayTagStackContainer&;

    [[nodiscard]]
    auto
    Get_StackCount(
        const FGameplayTag& InTag) const -> int32;

    [[nodiscard]]
    auto
    Get_ContainsTag(
        const FGameplayTag& InTag) const -> bool;

public:
    auto
    PreReplicatedRemove(
        const TArrayView<int32> InRemovedIndices,
        int32 InFinalSize) -> void;

    auto
    PostReplicatedAdd(
        const TArrayView<int32> InAddedIndices,
        int32 InFinalSize) -> void;

    auto
    PostReplicatedChange(
        const TArrayView<int32> InChangedIndices,
        int32 InFinalSize) -> void;

    auto
    NetDeltaSerialize(
        FNetDeltaSerializeInfo& InDeltaParams) -> bool;

private:
    UPROPERTY()
    TArray<FCk_GameplayTagStack> _Stacks;

private:
    TMap<FGameplayTag, int32> _TagToCountMap;

public:
    CK_PROPERTY_GET(_Stacks);
    CK_PROPERTY_GET(_TagToCountMap);
};

// --------------------------------------------------------------------------------------------------------------------

template<>
struct TStructOpsTypeTraits<FCk_GameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FCk_GameplayTagStackContainer>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};

// --------------------------------------------------------------------------------------------------------------------
// Algos

namespace ck::algo
{
    struct CKCORE_API MatchesGameplayTagStackNameExact
    {
    public:
        auto operator()(const FCk_GameplayTagStack& InStack) const -> bool
        {
            return InStack.Get_Tag().MatchesTagExact(_Name);
        }

    private:
        FGameplayTag _Name;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesGameplayTagStackNameExact, _Name)
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKCORE_API MatchesGameplayTagStackName
    {
    public:
        auto operator()(const FCk_GameplayTagStack& InStack) const -> bool
        {
            return InStack.Get_Tag().MatchesTag(_Name);
        }

    private:
        FGameplayTag _Name;

    public:
        CK_DEFINE_CONSTRUCTORS(MatchesGameplayTagStackName, _Name)
    };
}

// --------------------------------------------------------------------------------------------------------------------
