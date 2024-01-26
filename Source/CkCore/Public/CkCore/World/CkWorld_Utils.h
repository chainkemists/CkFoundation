#pragma once

#include "CkCore/Ensure/CkEnsure.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkWorld_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_World_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_World_UE);

public:
    static auto
    TryGet_GameWorld(
        const UObject* InObject) -> TOptional<const UWorld*>;

    static auto
    TryGet_WorldFromOuterRecursively(
        const UObject* InOuter) -> TOptional<const UWorld*>;

    static auto
    TryGet_FirstValidWorld(
        const UObject* InObject,
        std::function<bool(const UWorld*)> InPredicate) -> TOptional<const UWorld*>;

    static auto
    TryGet_MutableGameWorld(
        const UObject* InObject) -> TOptional<UWorld*>;

    static auto
    TryGet_MutableWorldFromOuterRecursively(
        const UObject* InOuter) -> TOptional<UWorld*>;

    static auto
    TryGet_MutableFirstValidWorld(
        const UObject* InObject,
        std::function<bool(UWorld*)> InPredicate) -> TOptional<UWorld*>;

    static auto
    Get_AllAvailableGameWorlds() -> TArray<UWorld*>;
};

// --------------------------------------------------------------------------------------------------------------------
