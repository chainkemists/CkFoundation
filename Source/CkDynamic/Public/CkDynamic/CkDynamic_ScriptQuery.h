#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDynamic_ScriptQuery.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

// --------------------------------------------------------------------------------------------------------------------
// AngelScript mixin over FCk_ScriptProcessorQuery, letting a script processor's Configure read as:
//   Query.ReadWrite(FFragment_X);   Query.Require(FTag_Y);   Query.Exclude(FTag_Z);   Query.NoEntities();
// Mirrors the Meta=(ScriptMixin="FCk_Handle") pattern on UCk_Utils_DynamicFragment_UE. Mutators reject a
// duplicate fragment type across all slots (keep-first + ensure) and an invalid type; NoEntities requires an
// empty slot list. The struct itself lives in CkEcs (see CkProcessor_ScriptQuery_Data.h) so the base-class
// Configure event can reference it without CkEcs depending on CkDynamic.
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_ScriptProcessorQuery"))
class CKDYNAMIC_API UCk_ScriptProcessorQuery_Mixin_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|ScriptProcessor")
    static void
    ReadWrite(
        UPARAM(ref) FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType);

    UFUNCTION(BlueprintCallable, Category = "Ck|ScriptProcessor")
    static void
    ReadOnly(
        UPARAM(ref) FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType);

    UFUNCTION(BlueprintCallable, Category = "Ck|ScriptProcessor")
    static void
    Require(
        UPARAM(ref) FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType);

    UFUNCTION(BlueprintCallable, Category = "Ck|ScriptProcessor")
    static void
    Exclude(
        UPARAM(ref) FCk_ScriptProcessorQuery& InQuery,
        const UScriptStruct* InType);

    UFUNCTION(BlueprintCallable, Category = "Ck|ScriptProcessor")
    static void
    NoEntities(
        UPARAM(ref) FCk_ScriptProcessorQuery& InQuery);
};
