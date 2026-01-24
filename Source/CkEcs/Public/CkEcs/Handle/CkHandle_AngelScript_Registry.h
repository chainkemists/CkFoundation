#pragma once

#include "CkEcs/Handle/CkHandle.h"

#if WITH_ANGELSCRIPT_CK

#include <AngelscriptBinds.h>

// --------------------------------------------------------------------------------------------------------------------

struct CKECS_API FCkAngelScript_HandleTypeInfo
{
    FString TypeName;
    FString ShortName;

    TFunction<bool(const FCk_Handle&)> IsValidAsType;
    TFunction<FCk_Handle(const FCk_Handle&)> Cast;
    TFunction<FCk_Handle(const FCk_Handle&)> CastChecked;

    TFunction<void()> TypeBindingsCallback;

    TArray<FString> RequiredFragments;
    FString Description;
    FString SourceAsset;

    bool IsDynamicHandle = false;

    /**
     * Optional callback to create custom FAngelscriptType for this handle.
     * If set, called during type binding creation.
     */
    TFunction<void(const FString& /*TypeName*/, const FString& /*ShortName*/)> OnCreateAngelscriptType;
};

// --------------------------------------------------------------------------------------------------------------------

class CKECS_API FCkAngelScript_HandleRegistry
{
public:
    // ------------------------------------------------
    // Registration
    // ------------------------------------------------

    /**
     * Register a deferred callback for static handle registration.
     * The callback will be invoked during PreCompile when Has/Cast/CastChecked are available.
     * Called by CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION macro.
     */
    static auto
    RegisterDeferredCallback(
        TFunction<void()> InCallback) -> void;

    /**
     * Register a static (C++ USTRUCT) handle type.
     * Called from within deferred callbacks after Has/Cast/CastChecked are defined.
     */
    static auto
    RegisterStaticHandle(
        const FString& InTypeName,
        const FString& InShortName,
        TFunction<bool(const FCk_Handle&)> InHasFunc,
        TFunction<FCk_Handle(const FCk_Handle&)> InCastFunc,
        TFunction<FCk_Handle(const FCk_Handle&)> InCastCheckedFunc,
        TFunction<void()> InTypeBindingsCallback = nullptr) -> bool;

    /**
     * Register a dynamic (data asset) handle type.
     * Called by FCkDynamic_HandleTypeRegistry.
     */
    static auto
    RegisterDynamicHandle(
        const FString& InTypeName,
        const FString& InShortName,
        TFunction<bool(const FCk_Handle&)> InValidator,
        const TArray<FString>& InRequiredFragments = {},
        const FString& InDescription = {},
        const FString& InSourceAsset = {}) -> bool;

    /**
     * Register new types incrementally at runtime.
     * Only registers types that aren't already registered.
     * Returns the number of newly registered types.
     */
    static auto
    RegisterNewTypesIncremental() -> int32;

    /**
     * Reset the bindings complete flag to allow re-binding.
     * Use with caution - primarily for editor refresh scenarios.
     */
    static auto
    ResetBindingsCompleteFlag() -> void;

    /**
     * Set a global callback for creating AngelScript types for dynamic handles.
     * Called by CkDynamic module to register its type factory.
     */
    static auto
    SetDynamicHandleTypeFactory(
        TFunction<void(const FString&, const FString&)> InFactory) -> void;

    // ------------------------------------------------
    // Queries
    // ------------------------------------------------

    static auto
    IsHandleTypeRegistered(
        const FString& InTypeName) -> bool;

    static auto
    GetHandleTypeInfo(
        const FString& InTypeName) -> const FCkAngelScript_HandleTypeInfo*;

    static auto
    FindByShortName(
        const FString& InShortName) -> const FCkAngelScript_HandleTypeInfo*;

    static auto
    GetAllRegisteredTypes() -> const TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>&;

    // ------------------------------------------------
    // Binding Lifecycle
    // ------------------------------------------------

    static auto
    EnsureCallbackRegistered() -> void;

    static auto
    EnsureAllBindingsComplete() -> void;

private:
    static auto
    RegisterHandleType(
        FCkAngelScript_HandleTypeInfo&& InTypeInfo) -> bool;

    static auto
    RegisterAllPendingTypes() -> void;

    static auto
    ExecuteDeferredCallbacks() -> void;

    static auto
    CreateTypeBindings(
        const FString& InTypeName) -> void;

    static auto
    CreateDynamicTypeValueClass(
        const FCkAngelScript_HandleTypeInfo& InTypeInfo) -> void;

    static auto
    BindBaseHandleMethods(
        const FCkAngelScript_HandleTypeInfo& InTypeInfo) -> void;

    static auto
    BindCrossHandleConversions() -> void;

    static auto
    BindBaseMixinMethods() -> void;

    static auto
    Get_RegisteredTypes() -> TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>&;

    static auto
    Get_PendingTypes() -> TArray<FCkAngelScript_HandleTypeInfo>&;

    static auto
    Get_DeferredCallbacks() -> TArray<TFunction<void()>>&;

    static auto
    Get_BoundConversionPairs() -> TSet<TPair<FString, FString>>&;

    static auto
    Get_DynamicHandleTypeFactory() -> TFunction<void(const FString&, const FString&)>&;

private:
    static inline FDelegateHandle _PreCompileDelegateHandle;
    static inline bool _BindingsComplete = false;
};

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK