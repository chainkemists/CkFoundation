#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class FProperty;
class UEnum;
class UScriptStruct;

namespace ck::dynamic
{
    // Registry access is game-thread-only, matching dynamic-fragment storage mutation and debugger inspection.

    /**
     * Cooked-safe labels for one dynamic-fragment type.
     *
     * Keys and labels are values on purpose: an AngelScript hot reload replaces its reflected objects and descriptor
     * graph. Nothing in this contract may retain those pointers beyond the registration call.
     */
    struct CKDYNAMIC_API FFragmentDisplaySchema
    {
        FString FragmentDisplayName;
        TMap<FName, FString> PropertyDisplayNames;
        TMap<FString, TMap<int64, FString>> EnumValueDisplayNames;
    };

    /**
     * Register an explicit producer-owned schema. Native registrations have precedence over generated AngelScript
     * schemas and are not removed when AngelScript recompiles.
     */
    CKDYNAMIC_API auto
    Register_NativeFragmentDisplaySchema(
        FString InFragmentTypePath,
        FFragmentDisplaySchema InSchema) -> bool;

    /** Symmetric producer cleanup for module shutdown, live reload, and isolated automation coverage. */
    CKDYNAMIC_API auto
    Unregister_NativeFragmentDisplaySchema(
        const FString& InFragmentTypePath) -> bool;

    CKDYNAMIC_API auto
    TryGet_NativeFragmentDisplaySchema(
        const FString& InFragmentTypePath,
        FFragmentDisplaySchema& OutSchema) -> bool;

    /** Copy a registered schema without exposing storage whose address can be invalidated by a refresh. */
    CKDYNAMIC_API auto
    TryGet_FragmentDisplaySchema(
        const FString& InFragmentTypePath,
        FFragmentDisplaySchema& OutSchema) -> bool;

    /**
     * Record a dynamic-fragment type at the first storage-id lookup. AngelScript schemas are generated lazily for
     * observed types; native types continue to use an explicit schema or deterministic reflected-name fallback.
     */
    CKDYNAMIC_API auto
    Observe_FragmentDisplaySchema(
        const UScriptStruct* InFragmentType) -> bool;

    CKDYNAMIC_API auto
    Resolve_FragmentDisplayName(
        const UScriptStruct* InFragmentType) -> FString;

    CKDYNAMIC_API auto
    Resolve_PropertyDisplayName(
        const UScriptStruct* InFragmentType,
        const FProperty* InProperty) -> FString;

    CKDYNAMIC_API auto
    Resolve_EnumValueDisplayName(
        const UScriptStruct* InFragmentType,
        const UEnum* InEnum,
        int64 InValue) -> FString;

    // Internal producer seam used by the AngelScript bridge and focused automation coverage. Replacing the AS map is
    // atomic and deliberately cannot affect the explicit native map.
    CKDYNAMIC_API auto
    Replace_AngelscriptFragmentDisplaySchemas(
        TMap<FString, FFragmentDisplaySchema> InSchemas) -> bool;

    CKDYNAMIC_API auto
    Get_ObservedFragmentDisplaySchemaPaths() -> TSet<FString>;

    CKDYNAMIC_API auto
    Get_AngelscriptFragmentDisplaySchemas() -> TMap<FString, FFragmentDisplaySchema>;

#if WITH_ANGELSCRIPT_CK
    CKDYNAMIC_API auto
    TryBuild_AngelscriptFragmentDisplaySchema(
        const UScriptStruct* InFragmentType,
        FFragmentDisplaySchema& OutSchema) -> bool;

    CKDYNAMIC_API auto
    Refresh_AngelscriptFragmentDisplaySchemas() -> bool;

    /**
     * Thread-safe entry point for producers that cannot promise a thread. AngelScript initializes on a worker in
     * cooked builds, so its PostCompile broadcast arrives off the game thread and calling the refresh directly there
     * fails the game-thread invariant instead of publishing anything.
     */
    CKDYNAMIC_API auto
    Request_RefreshAngelscriptFragmentDisplaySchemas() -> void;

    CKDYNAMIC_API auto
    Startup_AngelscriptFragmentDisplaySchemas() -> void;

    CKDYNAMIC_API auto
    Shutdown_AngelscriptFragmentDisplaySchemas() -> void;
#endif
}

// --------------------------------------------------------------------------------------------------------------------
