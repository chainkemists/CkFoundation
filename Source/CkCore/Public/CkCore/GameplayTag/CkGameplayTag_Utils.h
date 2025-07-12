#pragma once

#include "CkCore/GameplayTag/CkGameplayTagStack.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkGameplayTag_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FGameplayTagCountContainer;
struct FGameplayTagRequirements;

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_GameplayTag_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameplayTag_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get Are Tag Requirements Met",
              meta = (Keywords = "container, does"))
    static bool
    Get_AreTagRequirementsMet(
        const FGameplayTagRequirements& InTagRequirements,
        const FGameplayTagContainer& InTagsToTest);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get Do GameplayTag Containers Intersect",
              meta = (Keywords = "container, does"))
    static bool
    Get_DoContainersIntersect(
        const FGameplayTagContainer& A,
        FGameplayTagContainer B);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get Do GameplayTag Containers Intersect (Exact)",
              meta = (Keywords = "container, does"))
    static bool
    Get_DoContainersIntersect_Exact(
        const FGameplayTagContainer& A,
        FGameplayTagContainer B);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get All Intersecting Tags",
              meta = (Keywords = "container, does"))
    static FGameplayTagContainer
    Get_AllIntersectingTags(
        const FGameplayTagContainer& A,
        FGameplayTagContainer B);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Make Literal GameplayTag (From String)")
    static FGameplayTag
    Make_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Try Make Literal GameplayTag (From String)")
    static FGameplayTag
    TryMake_LiteralGameplayTag_FromString(
        const FString& InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Make Literal GameplayTag (From Name)")
    static FGameplayTag
    Make_LiteralGameplayTag_FromName(
        FName InTagNameAsString);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Try Make Literal GameplayTag (From Name)")
    static FGameplayTag
    TryMake_LiteralGameplayTag_FromName(
        FName InTagNameAsString);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get GameplayTag Leaf (As Name)")
    static FName
    Get_Leaf_AsName(
        FGameplayTag InGameplayTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get GameplayTag Leaf (As String)")
    static FString
    Get_Leaf_AsString(
        FGameplayTag InGameplayTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get GameplayTag Leaf (As Text)")
    static FText
    Get_Leaf_AsText(
        FGameplayTag InGameplayTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get GameplayTag Root (As Name)")
    static FName
    Get_Root_AsName(
        FGameplayTag InGameplayTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get GameplayTag Root (As String)")
    static FString
    Get_Root_AsString(
        FGameplayTag InGameplayTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get GameplayTag Root (As Tag)")
    static FGameplayTag
    Get_Root_AsTag(
        FGameplayTag InGameplayTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTag",
              DisplayName = "[Ck] Get GameplayTag Root (As Text)")
    static FText
    Get_Root_AsText(
        FGameplayTag InGameplayTag);

public:
    static auto
    Get_TagsWithCount_FromContainer(
        const FGameplayTagCountContainer& InTagContainer) -> TMap<FGameplayTag, int32>;

    template <typename... T_Tags>
    static auto
    Get_GameplayTagContainerFromTags(
        T_Tags... InTags) -> FGameplayTagContainer;

public:
    // This will create the tag in edtitor only if the tag does not exist. In non-editor builds, this will trigger
    // an ensure if the tag does not exist. In shipping builds, this will most likely crash if the tag does not exist.
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|EditorOnly",
        DisplayName = "[Ck] Resolve GameplayTag")
    static FGameplayTag
    ResolveGameplayTag(
        FName TagName,
        const FString& Comment = TEXT("Added via code"));
};

// --------------------------------------------------------------------------------------------------------------------

template <typename ... T_Tags>
auto
    UCk_Utils_GameplayTag_UE::
    Get_GameplayTagContainerFromTags(
        T_Tags... InTags)
        -> FGameplayTagContainer
{
    static_assert((std::is_same_v<T_Tags, FGameplayTag> && ...), "All parameters MUST be FGameplayTag");

    auto Container = FGameplayTagContainer{};
    (Container.AddTag(InTags), ...);
    return Container;
}

// --------------------------------------------------------------------------------------------------------------------


UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_GameplayTagStack_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameplayTagStack_UE);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Add Stack to GameplayTagStackContainer")
    static void
    Request_AddStack(
        UPARAM(ref) FCk_GameplayTagStackContainer& InStackContainer,
        const FCk_GameplayTagStack& InStack);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Remove Stack from GameplayTagStackContainer")
    static void
    Request_RemoveStack(
        UPARAM(ref) FCk_GameplayTagStackContainer& InStackContainer,
        const FCk_GameplayTagStack& InStack);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Get Contains Tag in GameplayTagStackContainer")
    static bool
    Get_ContainsTag(
        const FCk_GameplayTagStackContainer& InStackContainer,
        const FGameplayTag& InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Get Stack Count in GameplayTagStackContainer")
    static int32
    Get_StackCount(
        const FCk_GameplayTagStackContainer& InStackContainer,
        const FGameplayTag& InTag);

private:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Make GameplayTagStackContainer",
              meta = (NativeMakeFunc))
    static FCk_GameplayTagStackContainer
    Make_GameplayTagStackContainer(
        const TArray<FCk_GameplayTagStack>& InStacks);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Break GameplayTagStackContainer",
              meta = (NativeBreakFunc))
    static TArray<FCk_GameplayTagStack>
    Break_GameplayTagStackContainer(
        const FCk_GameplayTagStackContainer& InStackContainer);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|GameplayTagStack",
              DisplayName = "[Ck] Break GameplayTagStackContainer (As Map)")
    static TMap<FGameplayTag, int32>
    Break_GameplayTagStackContainer_AsMap(
        const FCk_GameplayTagStackContainer& InStackContainer);
};

// --------------------------------------------------------------------------------------------------------------------