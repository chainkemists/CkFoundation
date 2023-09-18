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
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_Handle_IsValid(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "==", KeyWords = "==,equal"))
    static bool
    Get_Handle_IsEqual(
        const FCk_Handle& InHandleA,
        const FCk_Handle& InHandleB);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "!=", KeyWords = "!=,not,equal"))
    static bool
    Get_Handle_IsNotEqual(
        const FCk_Handle& InHandleA,
        const FCk_Handle& InHandleB);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Handle",
              meta = (DisplayName = "Handle To Text", CompactNodeTitle = "->", BlueprintAutocast))
    static FText
    Conv_HandleToText(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Handle",
              meta = (DisplayName = "Handle To String", CompactNodeTitle = "->", BlueprintAutocast))
    static FString
    Conv_HandleToString(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Handle", meta = (NativeBreakFunc))
    static void
    Break_Handle(
        const FCk_Handle& InHandle,
        FCk_Entity& OutEntity);

};

// --------------------------------------------------------------------------------------------------------------------
