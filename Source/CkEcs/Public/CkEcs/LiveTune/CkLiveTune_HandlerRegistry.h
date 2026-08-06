#pragma once

// Per-feature re-apply registry for LiveTune — the editor-only change transport for live-tunable feature
// params: an editor-time edit of a linked tuning asset is dispatched back onto live entities through the
// handler registered for the params TYPE (docs/specs/2026-08-05-LiveTune-design.md §4.4/§5). Three tiers:
//   ViaReplace — live-read features; Replace<Params> IS the re-apply (optional PostReplace fixup).
//   ViaRequest — setup-baked features that own an in-place rebuild path; route through their Request_*.
//   ViaRebuild — cascading setup; persistence Produce -> destroy feature subtree -> re-Add -> hydrate,
//                reusing the feature's FCk_PersistenceHandlerRegistry entries wholesale.
// Registration is EXPLICIT opt-in — one line in the feature's _Fragment.cpp; an edit whose params type has
// no handler logs Display and does nothing. Outside WITH_EDITOR the Register_* shapes are empty inlines,
// so registration call sites stay #if-free.

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"

#include <InstancedStruct.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_LiveTune_ApplyTier : uint8
{
    ViaReplace,
    ViaRequest,
    ViaRebuild
};

// Scope of a ViaRebuild re-apply: Feature destroys + re-Adds the feature's child subtree; Entity is the
// full recipe-respawn fallback, valid ONLY for RuntimeSpawned entities — the rebuild driver refuses
// anything without a spawn recipe loudly.
enum class ECk_LiveTune_RebuildScope : uint8
{
    Feature,
    Entity
};

// --------------------------------------------------------------------------------------------------------------------

class CKECS_API FCk_LiveTuneHandlerRegistry
{
public:
    using FApplyFn       = TFunction<void(FCk_Handle& Entity, const FInstancedStruct& FreshParams)>;
    using FHasFragmentFn = TFunction<bool(const FCk_Handle& Entity)>;
    using FReAddFn       = TFunction<FCk_Handle(FCk_Handle& Owner, const FInstancedStruct& FreshParams)>;
    using FPostReplaceFn = TFunction<void(FCk_Handle& Entity)>;
    using FCaptureFn     = TFunction<TOptional<FInstancedStruct>(FCk_Handle& LinkedEntity, const FInstancedStruct& FreshParams)>;

    struct FHandler
    {
        ECk_LiveTune_ApplyTier Tier = ECk_LiveTune_ApplyTier::ViaReplace;

        // Set for ViaReplace only — the one tier whose contract REQUIRES the params fragment on the
        // entity, so Link can validate against it (ViaRequest/ViaRebuild features may keep no params
        // fragment at all — FloatAttribute is fully decomposed at Add).
        FHasFragmentFn HasFragment;

        // Direct re-apply, synthesized by Register_ViaReplace / Register_ViaRequest. Unset for
        // ViaRebuild — that tier routes through the rebuild driver, which consumes the slots below.
        FApplyFn Apply;

        ECk_LiveTune_RebuildScope RebuildScope = ECk_LiveTune_RebuildScope::Feature;
        FReAddFn ReAdd;

        // Optional capture override for the rebuild driver. Unset = the driver sweeps every
        // save-participating FCk_PersistenceHandlerRegistry Produce over the LINKED entity. Save payloads
        // restore ABSOLUTELY, which is right for load (the recipe rebuilt the original config) but reverts
        // a retune for any CONFIG value the payload carries — a feature whose payload mixes config with
        // runtime state sets this to strip the config entries (the attribute shape: keep the Current
        // entry verbatim — its "base" IS the live value — drop the Min/Max config entries so the fresh
        // clamps win). Re-application always rides FProcessor_Hydration_Dispatch either way.
        FCaptureFn CaptureOverride;
    };

    // Required-slot wrapper: no default constructor, so OMITTING the slot in the braced args below is a
    // COMPILE ERROR rather than a runtime ensure (mirrors FCk_PersistenceHandlerRegistry::FRequired*).
    template <typename T_Fn>
    struct TRequired
    {
        T_Fn Value;
        TRequired() = delete;
        template <typename T, typename = std::enable_if_t<std::is_constructible_v<T_Fn, T&&>>>
        TRequired(T&& InFn) : Value(Forward<T>(InFn)) {}
    };

    // Per-shape designated-init args (field order = designator order). Lambdas are TYPED on the params
    // struct at the call site; the registry stores them type-erased.
    template <typename T_Params>
    struct TArgs_ViaReplace
    {
        FPostReplaceFn PostReplace{};
    };

    template <typename T_Params>
    struct TArgs_ViaRequest
    {
        TRequired<TFunction<void(FCk_Handle& Entity, const T_Params& FreshParams)>> Apply;
    };

    template <typename T_Params>
    struct TArgs_ViaRebuild
    {
        ECk_LiveTune_RebuildScope Scope = ECk_LiveTune_RebuildScope::Feature;
        TRequired<TFunction<FCk_Handle(FCk_Handle& Owner, const T_Params& FreshParams)>> ReAdd;
        TFunction<TOptional<FInstancedStruct>(FCk_Handle& LinkedEntity, const T_Params& FreshParams)> Capture{};
    };

#if WITH_EDITOR
    // Named registration shapes — bodies live in CkLiveTune_HandlerRegistry.inl.h so they instantiate
    // where T_Params is complete; include both headers in the registering .cpp. Safe to call during
    // static init (lazy type resolution, mirroring FCk_PersistenceHandlerRegistry).
    template <typename T_Params>
    static auto Register_ViaReplace(TArgs_ViaReplace<T_Params> InArgs = {}) -> void;

    template <typename T_Params>
    static auto Register_ViaRequest(TArgs_ViaRequest<T_Params> InArgs) -> void;

    template <typename T_Params>
    static auto Register_ViaRebuild(TArgs_ViaRebuild<T_Params> InArgs) -> void;

    static auto
    Find(
        const UScriptStruct* InType) -> const FHandler*;

private:
    using FTypeResolver = TFunction<UScriptStruct*()>;

    struct FLazyEntry
    {
        FTypeResolver TypeResolver;
        FHandler Handler;
    };

    static auto
    RegisterLazy(
        FTypeResolver InTypeResolver,
        FHandler InHandler) -> void;

    static auto
    ResolvePending() -> void;

    static TMap<const UScriptStruct*, FHandler> _Handlers;
    static TArray<FLazyEntry> _PendingHandlers;
#else
    template <typename T_Params>
    static auto Register_ViaReplace(TArgs_ViaReplace<T_Params> = {}) -> void {}

    template <typename T_Params>
    static auto Register_ViaRequest(TArgs_ViaRequest<T_Params>) -> void {}

    template <typename T_Params>
    static auto Register_ViaRebuild(TArgs_ViaRebuild<T_Params>) -> void {}
#endif
};

// --------------------------------------------------------------------------------------------------------------------
