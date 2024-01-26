#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

#include <Misc/Optional.h>
#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDebug_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DebugName_Verbosity : uint8
{
    FullName,
    ShortName
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Debug_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Debug_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Get Debug Name")
    static FName
    Get_DebugName(
        const UObject* InObject,
        ECk_DebugName_Verbosity InNameVerbosity = ECk_DebugName_Verbosity::FullName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Get Debug Name (As String)")
    static FString
    Get_DebugName_AsString(
        const UObject* InObject,
        ECk_DebugName_Verbosity InNameVerbosity = ECk_DebugName_Verbosity::FullName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Get Debug Name (As Text)")
    static FText
    Get_DebugName_AsText(
        const UObject* InObject,
        ECk_DebugName_Verbosity InNameVerbosity = ECk_DebugName_Verbosity::FullName
    );
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Debug_StackTrace_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Debug_StackTrace_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Get Stack Trace")
    static FString
    Get_StackTrace(int32 InSkipFrames = 1);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Get Blueprint Stack Trace (As Array)")
    static TArray<FString>
    Get_StackTrace_Blueprint_AsArray();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Debug",
              DisplayName = "[Ck] Get Blueprint Stack Trace (As String)")
    static FString
    Get_StackTrace_Blueprint_AsString();

public:
    static auto Get_BlueprintContext() -> TOptional<FString>;

    static auto Get_StackTrace_Blueprint(ck::type_traits::AsArray) -> TArray<FString>;
    static auto Get_StackTrace_Blueprint(ck::type_traits::AsString) -> FString;

    static auto Try_BreakInScript(const UObject* InContext) -> void;

private:
    inline static UObject* _LastStackTraceContextObject = nullptr;
};

// --------------------------------------------------------------------------------------------------------------------
