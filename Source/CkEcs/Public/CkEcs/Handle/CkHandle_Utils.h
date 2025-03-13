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
              meta = (CompactNodeTitle = "BREAK", DevelopmentOnly))
    static void
    Debug_Handle(
        const FCk_Handle& InHandle);

private:
    // C++ users should use CK_BREAK_IF_HANDLE_NAME
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Handle",
              DisplayName = "[Ck] Break if Handle DebugName Contains",
              meta     = (DevelopmentOnly))
    static void
    BreakIfHandleDebugNameContains(
        const FCk_Handle& InHandle,
        const FString& InTextToFind);

public:
    static auto
    Set_DebugName(
        FCk_Handle& InHandle,
        FName InDebugName,
        ECk_Override InOverride = ECk_Override::Override) -> void;

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Entity Debug Name",
              Category = "Ck|Utils|Handle")
    static FName
    Get_DebugName(
        const FCk_Handle& InHandle);

public:
    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Break Handle",
              Category = "Ck|Utils|Handle",
              meta = (NativeBreakFunc))
    static void
    Break_Handle(
        const FCk_Handle& InHandle,
        FCk_Entity& OutEntity);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid Handle",
              Category = "Ck|Utils|Handle",
              meta = (CompactNodeTitle = "INVALID_Handle", Keywords = "make"))
    static const FCk_Handle&
    Get_InvalidHandle();

    UFUNCTION(BlueprintPure,
              CustomThunk,
              Displayname = "[Ck] Get Raw Handle",
              Category = "Ck|Utils|Handle",
              meta=(CompactNodeTitle = "<AsRawHandle>", CustomStructureParam = "InHandle", BlueprintInternalUseOnly = true))
    static FCk_Handle
    Get_RawHandle(
        UStruct* InHandle);
    DECLARE_FUNCTION(execGet_RawHandle);
};

// --------------------------------------------------------------------------------------------------------------------

#define CK_BREAK_IF_HANDLE_NAME(_Handle_, _TextToFind_)\
{ \
    auto _DebugName_ = UCk_Utils_Handle_UE::Get_DebugName(_Handle_).ToString(); \
    auto _ToFind_ = _TextToFind_; \
    if (_DebugName_.Contains(_ToFind_)) \
    { \
        CK_TRIGGER_ENSURE(TEXT("Breaking because we FOUND [{}] in [{}]"), _ToFind_, _DebugName_); \
    } \
}

// --------------------------------------------------------------------------------------------------------------------
