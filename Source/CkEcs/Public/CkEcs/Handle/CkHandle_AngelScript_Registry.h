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

    /**
     * The C++ type name of this handle's parent in the typesafe handle inheritance chain
     * (e.g. "FCk_Handle_Inventory" for FCk_Handle_Inventory_Spatial). Empty when the handle
     * inherits directly from FCk_Handle_TypeSafe — the universal FCk_Handle is the implicit
     * root for every typesafe handle and is not stored here.
     *
     * Populated by CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION via
     * ck::details::Get_MixinParentTypeName<T>(), which keys off the
     * `MixinParentHandle` typedef planted by CK_GENERATED_BODY_HANDLE_DERIVED.
     *
     * Drives mixin-method propagation across handle inheritance chains in
     * BindBaseMixinMethods() — methods bound to a parent handle are re-bound onto the
     * derived handle so AngelScript callers don't have to cast back to the parent.
     */
    FString MixinParentTypeName;

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
        TFunction<void()> InTypeBindingsCallback = nullptr,
        const FString& InMixinParentTypeName = {}) -> bool;

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
     * Replace the validator + cast lambdas + metadata of an already-registered
     * dynamic handle type, in place. Useful when an existing handle's
     * UCkDynamic_HandleDefinition has its RequiredFragments changed (or when
     * the self-heal dispatcher needs to upgrade a permissive stub registration
     * to a strict validator sourced from a now-discoverable data asset).
     *
     * Pointer stability: the TypeInfo stored in Get_RegisteredTypes() is
     * mutated in place. AS-bound methods read it via a stable raw pointer
     * (held in userData / AuxData), so they pick up the new validator
     * automatically on the next call — no AS-engine re-registration needed.
     *
     * Returns false if the type isn't registered or if InTypeName is empty.
     * Returns true on successful update.
     */
    static auto
    UpdateExistingDynamicHandle(
        const FString& InTypeName,
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

    /**
     * Bind opImplConv (const + non-const) on each derived handle type for every typesafe
     * ancestor in its MixinParentHandle chain (excluding the universal FCk_Handle root, which
     * is wired by CreateDynamicTypeValueClass / CK_REGISTER_ANGELSCRIPT_HANDLE_CONVERSION).
     *
     * Derived handles thus implicitly convert to any parent typesafe handle in AngelScript
     * — e.g. FCk_Handle_Inventory_DataOnly is accepted where FCk_Handle_Inventory& is
     * expected, without an explicit As_Inventory(...).
     *
     * Validation note: implicit parent conversion is UNCHECKED — bytes are forwarded as-is,
     * no CastChecked / fragment-presence ensure runs at the call boundary. The downstream
     * util ensures when it touches state. Call sites that want the boundary diagnostic
     * should keep using As_Parent() explicitly.
     */
    static auto
    BindParentChainConversions() -> void;

    static auto
    Get_RegisteredTypes() -> TMap<FString, TSharedPtr<FCkAngelScript_HandleTypeInfo>>&;

    static auto
    Get_PendingTypes() -> TArray<FCkAngelScript_HandleTypeInfo>&;

    static auto
    Get_DeferredCallbacks() -> TArray<TFunction<void()>>&;

    static auto
    Get_BoundConversionPairs() -> TSet<TPair<FString, FString>>&;

    static auto
    Get_BoundMixinMethods() -> TSet<FString>&;

    /**
     * Per-pair dedup for parent-chain implicit conversions. Key shape: "{Derived}->{Ancestor}".
     * Cleared from ResetBindingsCompleteFlag so late-registered children re-bind cleanly.
     */
    static auto
    Get_BoundParentConversions() -> TSet<FString>&;

    /**
     * Shared "warned-once-per-type" set across BindParentChainConversions and BindBaseMixinMethods.
     * Cycles / missing-parent issues in the MixinParentHandle chain log a single warning per
     * offending type regardless of which pass discovers them first.
     */
    static auto
    Get_WarnedMixinTypes() -> TSet<FString>&;

    static auto
    Get_DynamicHandleTypeFactory() -> TFunction<void(const FString&, const FString&)>&;

private:
    static inline FDelegateHandle _PreCompileDelegateHandle;
    static inline bool _BindingsComplete = false;
};

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_ANGELSCRIPT_CK