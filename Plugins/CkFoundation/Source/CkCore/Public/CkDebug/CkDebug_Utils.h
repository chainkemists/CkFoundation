#pragma once

#include "CkMacros/CkMacros.h"
#include "CkTypeTraits/CkTypeTraits.h"

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
              Category = "Ck|Debug|Utils")
    static FName
	Get_DebugName(const UObject* InObject,
        ECk_DebugName_Verbosity InNameVerbosity = ECk_DebugName_Verbosity::FullName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Debug|Utils")
    static FString
	Get_DebugName_AsString(const UObject* InObject,
        ECk_DebugName_Verbosity InNameVerbosity = ECk_DebugName_Verbosity::FullName);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Debug|Utils")
    static FText
	Get_DebugName_AsText(const UObject* InObject,
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
              Category = "Ck|Debug|Utils")
    static FString
	Get_StackTrace(int32 InSkipFrames = 1);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Debug|Utils")
    static TArray<FString>
    Get_StackTrace_Blueprint_AsArray();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Debug|Utils")
    static FString
    Get_StackTrace_Blueprint_AsString();

public:
    static auto Get_BlueprintContext() -> TOptional<FString>;

    static auto Get_StackTrace_Blueprint(ck::type_traits::as_array) -> TArray<FString>;
    static auto Get_StackTrace_Blueprint(ck::type_traits::as_string) -> FString;

    static auto Try_BreakInScript(const UObject* InContext) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
