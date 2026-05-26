#pragma once

#include "CkDependencyProvider_Common.h"
#include "CkDependencyProvider_Request.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkDependencyProvider_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// AS- and Blueprint-facing surface for the dependency-injection registry.
// Every mutating/query static takes the scope as an enum parameter and
// dispatches to the matching subsystem.
//
// Resolution: every static takes a WorldContextObject and resolves the
// subsystem via the resolved UWorld / UGameInstance. Never read
// GEngine->GetWorld() or other globals from here — that breaks PIE with
// multiple worlds open.
//
// Diagnostic helpers (Get_InjectionSite*) don't take scope — they walk
// FCk_InjectionCache for a UClass and are scope-agnostic. Intended for
// the Phase 2 validator and AS autotests, not production gameplay logic.
UCLASS(NotBlueprintable)
class CKECS_API UCk_Utils_DependencyProvider_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DependencyProvider_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Register",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Register(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        const FCk_Request_DependencyProvider_Register& InRequest);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Unregister",
              meta = (WorldContext = "InWorldContextObject"))
    static void
    Unregister(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        UScriptStruct* InHandleType);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Resolve",
              meta = (WorldContext = "InWorldContextObject"))
    static FCk_Handle
    Resolve(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        UScriptStruct* InHandleType);

    // Diagnostic — count of pending callbacks queued against the given
    // scope/type. 0 if nothing pending.
    UFUNCTION(BlueprintPure, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Get Pending Count",
              meta = (WorldContext = "InWorldContextObject"))
    static int32
    Get_PendingCount(
        const UObject* InWorldContextObject,
        ECk_DependencyProvider_Scope InScope,
        UScriptStruct* InHandleType);

    // -------- Phase 0 spike + diagnostic surface --------------------------------------------
    // These walk FCk_InjectionCache for an EntityScript class. Scope-agnostic
    // — the cache holds plans across all scopes. Used by the Phase 0 spike
    // to verify AS UPROPERTY meta tags survive the AS->UHT->FProperty
    // pipeline, and by the Phase 2 validator for eager world-startup
    // diagnostics.

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Get Injection Site Count")
    static int32
    Get_InjectionSiteCount_ForClass(
        UClass* InScriptClass);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Get Injection Site Property Name")
    static FName
    Get_InjectionSitePropertyName_ForClass(
        UClass* InScriptClass,
        int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Get Injection Site Scope")
    static ECk_DependencyProvider_Scope
    Get_InjectionSiteScope_ForClass(
        UClass* InScriptClass,
        int32 InIndex);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|DependencyProvider",
              DisplayName = "[Ck][DI] Get Injection Site Handle Type")
    static UScriptStruct*
    Get_InjectionSiteHandleType_ForClass(
        UClass* InScriptClass,
        int32 InIndex);
};
