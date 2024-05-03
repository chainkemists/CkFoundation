#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkString_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKCORE_API UCk_Utils_String_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_String_UE);

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|String",
              DisplayName = "[Ck] Get Skel Prefix",
              meta = (CompactNodeTitle = "'SKEL_'"))
    static FName
    Get_Skel_Prefix();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|String",
              DisplayName = "[Ck] Get Reinst Prefix",
              meta = (CompactNodeTitle = "'REINST_'"))
    static FName
    Get_Reinst_Prefix();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|String",
              DisplayName = "[Ck] Get Deadclass Prefix",
              meta = (CompactNodeTitle = "'DEADCLASS_'"))
    static FName
    Get_Deadclass_Prefix();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|String",
              DisplayName = "[Ck] Get Compiled From Blueprint Prefix",
              meta = (CompactNodeTitle = "'_C'"))
    static FName
    Get_CompiledFromBlueprint_Suffix();

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|String",
              DisplayName = "[Ck] Get Invalid Name",
              meta = (CompactNodeTitle = "INVALID_Name"))
    static FName
    Get_InvalidName();

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|String",
              DisplayName = "[Ck] Get Symbol N times")
    static FString
    Get_SymbolNTimes(
        const FString& InSymbol,
        int32 InCount);
};

// --------------------------------------------------------------------------------------------------------------------
