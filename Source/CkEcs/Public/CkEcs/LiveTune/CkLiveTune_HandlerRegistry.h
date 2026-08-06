#pragma once

// Per-feature re-apply registry for LiveTune — the editor-only change transport for live-tunable feature
// params: an editor-time edit of a linked tuning asset is dispatched back onto live entities through the
// handler registered for the params TYPE (docs/specs/2026-08-05-LiveTune-design.md §4.4/§5). Two shapes:
//   Register           — the re-apply is ONE synchronous call. Defaults to Replace<Params>, which is the
//                        whole re-apply for a live-read feature; pass .Apply to route through the
//                        feature's own reconfigure request instead.
//   Register_ViaRebuild — cascading setup, and NOT a call: the feature hands over a ReAdd callback and the
//                        subsystem drives a multi-frame capture -> destroy -> re-Add -> hydrate protocol,
//                        reusing the feature's FCk_PersistenceHandlerRegistry entries wholesale.
// Registration is EXPLICIT opt-in — one line in the feature's _Fragment.cpp. It is also what makes a
// feature linkable at all: Link ENSUREs on an unregistered params type, because a stamp that silently
// swallows every edit is a failure the caller cannot see. Outside WITH_EDITOR the Register shapes are
// empty inlines, so registration call sites stay #if-free.

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"

#include <InstancedStruct.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

// Direct is one synchronous Apply call. Rebuild is a protocol the subsystem drives across frames — which is
// why it cannot be expressed as an Apply lambda: it needs the pending-rebuild queue, the GC pins and the
// destruction-keyed re-Add, none of which a feature can reach from inside a call.
enum class ECk_LiveTune_ApplyKind : uint8
{
    Direct,
    Rebuild
};

// A details-panel slider drag broadcasts EPropertyChangeType::Interactive once per tick. Whether a re-apply
// belongs on that path is a property of its COST, not of how it is written — a cheap reconfigure request is
// as previewable as a fragment swap, and gating on the shape of the handler would needlessly deny it.
enum class ECk_LiveTune_ScrubPolicy : uint8
{
    // Resolve from the re-apply: the default Replace path is a fragment swap and previews live; a custom
    // Apply is assumed to touch state too expensive to redo per drag-frame until it says otherwise.
    // Rebuild always resolves to OnCommit — a rebuild per drag-frame is the storm this policy exists to stop.
    Auto,
    DuringScrub,
    OnCommit
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
    using FPostApplyFn   = TFunction<void(FCk_Handle& Entity)>;
    using FCaptureFn     = TFunction<TOptional<FInstancedStruct>(FCk_Handle& LinkedEntity, const FInstancedStruct& FreshParams)>;

    struct FHandler
    {
        ECk_LiveTune_ApplyKind Kind = ECk_LiveTune_ApplyKind::Direct;

        // Always resolved to DuringScrub or OnCommit by the time it lands here — never Auto.
        ECk_LiveTune_ScrubPolicy ScrubPolicy = ECk_LiveTune_ScrubPolicy::OnCommit;

        // Set only when the re-apply is the DEFAULT Replace, the one contract that REQUIRES the params
        // fragment on the entity, so Link can validate against it. A feature with a custom Apply may keep
        // no params fragment at all (FloatAttribute is fully decomposed at Add).
        FHasFragmentFn HasFragment;

        // The whole re-apply for Kind::Direct. Unset for Kind::Rebuild — that shape routes through the
        // rebuild driver, which consumes the slots below.
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
    struct TArgs
    {
        // Unset = the re-apply IS Replace<T_Params>, which is the whole story for a live-read feature.
        // Set it when the params were baked into state a fragment write cannot reach (an external body, a
        // UObject) and the feature owns an in-place reconfigure request to route through instead.
        TFunction<void(FCk_Handle& Entity, const T_Params& FreshParams)> Apply{};

        // Runs after Apply, default or custom. For state Add DERIVED from the params and cached elsewhere
        // — a tag, a chrono, a handle — which no params write refreshes on its own.
        FPostApplyFn PostApply{};

        ECk_LiveTune_ScrubPolicy ScrubPolicy = ECk_LiveTune_ScrubPolicy::Auto;
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
    static auto Register(TArgs<T_Params> InArgs = {}) -> void;

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
    static auto Register(TArgs<T_Params> = {}) -> void {}

    template <typename T_Params>
    static auto Register_ViaRebuild(TArgs_ViaRebuild<T_Params>) -> void {}
#endif
};

// --------------------------------------------------------------------------------------------------------------------
