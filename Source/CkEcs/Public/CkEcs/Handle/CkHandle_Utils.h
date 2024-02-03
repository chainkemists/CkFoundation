#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkHandle_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_Handle_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Handle_UE);

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Is Valid (Handle)",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Handle == Handle",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    IsEqual(
        const FCk_Handle& InHandleA,
        const FCk_Handle& InHandleB);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Handle != Handle",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    IsNotEqual(
        const FCk_Handle& InHandleA,
        const FCk_Handle& InHandleB);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Handle -> Text",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FText
    Conv_HandleToText(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Handle -> String",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "->", BlueprintAutocast))
    static FString
    Conv_HandleToString(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintCallable,
              DisplayName = "[Ck] Debug Handle",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "BREAK", DevevelopmentOnly))
    static void
    Debug_Handle(
        const FCk_Handle& InHandle);

public:
    static auto
    Set_DebugName(
        FCk_Handle InHandle,
        FName InDebugName) -> void;

private:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Handle",
              Category = "Ck|Utils|Handle",
              meta = (NativeBreakFunc))

    static void
    Break_Handle(
        const FCk_Handle& InHandle,
        FCk_Entity& OutEntity);

    UFUNCTION(BlueprintPure,
        CustomThunk,
        DisplayName = "[Ck] Handle_TypeSafe",
        Category = "Ck|Utils|Handle",
        meta=(CompactNodeTitle="ToHandle", NativeBreakFunc, CustomStructureParam = "InHandle"))
    static void
    Conv_HandleTypeSafeToHandle(
        UStruct* InHandle,
        FCk_Handle& OutHandle);
    DECLARE_FUNCTION(execConv_HandleTypeSafeToHandle);
};

// --------------------------------------------------------------------------------------------------------------------
