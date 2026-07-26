#pragma once

// Transport-neutral persistence handler registry: the SAME registered projection (Produce + Apply handlers) serves
// the net wire and the save/load path. Net -> Persistence is the only allowed dependency direction — nothing here
// may include Net/.

#include "CkEcs/Handle/CkHandle.h"

#include <InstancedStruct.h>
#include <Misc/Optional.h>

#include <type_traits>

// --------------------------------------------------------------------------------------------------------------------

// Result of FHandler::NetApply / HydrationApply. NotReady = the targeted feature is not composed on this entity
// yet; the entry stays pending and the dispatcher retries next tick. Rejected = permanently invalid, dropped
// immediately; never a retry state.
enum class ECk_Persistence_ApplyResult : uint8
{
    Applied,
    NotReady,
    Rejected
};

// --------------------------------------------------------------------------------------------------------------------

class CKECS_API FCk_PersistenceHandlerRegistry
{
public:
    struct FHandler
    {
        // NET-receive apply, dispatched deferred by FProcessor_ReplicatedFragments_Dispatch after
        // OnConstructed-driven composition — NEVER runs on the loading authority, and must never compose the
        // feature itself. OldData is unset on first application, else the last APPLIED data.
        TFunction<ECk_Persistence_ApplyResult(FCk_Handle& Entity,
                        const FInstancedStruct& NewData,
                        const TOptional<FInstancedStruct>& OldData)> NetApply;

        // Optional — dispatched (deferred) on NET removal. Like NetApply, never on the loading authority.
        TFunction<void(FCk_Handle& Entity)> NetRemove;

        // LOAD-PATH apply (authority-side hydration) via FProcessor_Hydration_Dispatch; OldData is always
        // unset (no coalescing). REQUIRED whenever Produce is set — reuse NetApply when it is authority-safe.
        TFunction<ECk_Persistence_ApplyResult(FCk_Handle& Entity,
                        const FInstancedStruct& NewData,
                        const TOptional<FInstancedStruct>& OldData)> HydrationApply;

        // Save-capture emitter, READ-ONLY by contract: the feature's payload, or unset when the feature is
        // absent. A SET-but-empty payload is meaningful (seeds an empty container); only UNSET means "absent".
        // Presence of Produce IS save participation. Authoring recipe: CkSnapshot/Claude.md.
        TFunction<TOptional<FInstancedStruct>(FCk_Handle& Entity)> Produce;
    };

    using FTypeResolver = TFunction<UScriptStruct*()>;

    /** Immediate registration — use only when the UScriptStruct* is known to be ready. */
    static auto
    Register(
        const UScriptStruct* InType,
        FHandler InHandler) -> void;

    /** Lazy registration — type is resolved on first Find(). Safe to call during static init. */
    static auto
    RegisterLazy(
        FTypeResolver InTypeResolver,
        FHandler InHandler) -> void;

    /**
     * Typed lazy registration. Resolves the payload type lazily via T_RepData::StaticStruct(), then forwards to
     * RegisterLazy. Body lives in CkPersistenceHandlerRegistry.inl.h so it instantiates where T_RepData is complete.
     */
    template <typename T_RepData>
    static auto
    RegisterLazyTyped(
        FHandler InHandler) -> void;

    // ---- Named participation shapes (compile-visible intent) --------------------------------------------------------
    // Prefer these over hand-building an FHandler: each takes a designated-init args struct, so every lambda is
    // LABELED at the call site and required slots are compile-enforced (see FRequired* below).

    using FApplyFn   = TFunction<ECk_Persistence_ApplyResult(FCk_Handle& Entity,
                            const FInstancedStruct& NewData, const TOptional<FInstancedStruct>& OldData)>;
    using FRemoveFn  = TFunction<void(FCk_Handle& Entity)>;
    using FProduceFn = TFunction<TOptional<FInstancedStruct>(FCk_Handle& Entity)>;

    // Required-slot wrappers: no default constructor, so OMITTING one of these in the braced args below is a COMPILE
    // ERROR rather than a runtime ensure. The is_constructible constraint excludes the wrapper type itself, so the
    // implicit copy/move constructors still apply when the args struct is moved.
    struct FRequiredProduce
    {
        FProduceFn Value;
        FRequiredProduce() = delete;
        template <typename T, typename = std::enable_if_t<std::is_constructible_v<FProduceFn, T&&>>>
        FRequiredProduce(T&& InFn) : Value(Forward<T>(InFn)) {}
    };
    struct FRequiredApply
    {
        FApplyFn Value;
        FRequiredApply() = delete;
        template <typename T, typename = std::enable_if_t<std::is_constructible_v<FApplyFn, T&&>>>
        FRequiredApply(T&& InFn) : Value(Forward<T>(InFn)) {}
    };

    // Per-shape designated-init args. Field order = the order designators must be written (C++ requires aggregate
    // designators in declaration order). Each struct exposes ONLY the slots its shape allows.
    struct FArgs_NetOnly
    {
        FRequiredApply NetApply;
        FRemoveFn      NetRemove{};
    };
    struct FArgs_SaveOnly
    {
        FRequiredProduce Produce;
        FRequiredApply   HydrationApply;
    };
    struct FArgs_NetAndSave_SharedApply
    {
        FRequiredProduce Produce;
        FRequiredApply   SharedApply;
        FRemoveFn        NetRemove{};
    };
    struct FArgs_NetAndSave_SplitApply
    {
        FRequiredProduce Produce;
        FRequiredApply   NetApply;
        FRequiredApply   HydrationApply;
        FRemoveFn        NetRemove{};
    };

    // Wire-only participation (never in the save file).
    template <typename T_RepData>
    static auto Register_NetOnly(FArgs_NetOnly InArgs) -> void;

    // Save-only participation (never rides a replicated container); Produce AND HydrationApply both required.
    template <typename T_RepData>
    static auto Register_SaveOnly(FArgs_SaveOnly InArgs) -> void;

    // Both transports, one authority-safe applier serving NetApply AND HydrationApply.
    template <typename T_RepData>
    static auto Register_NetAndSave_SharedApply(FArgs_NetAndSave_SharedApply InArgs) -> void;

    // Both transports, distinct appliers (net Apply is client-coupled — the TagSet shape).
    template <typename T_RepData>
    static auto Register_NetAndSave_SplitApply(FArgs_NetAndSave_SplitApply InArgs) -> void;

    /**
     * Resolves pending registrations, then returns the payload types of every save-participating handler — a
     * Produce (the payload emitter) paired with a HydrationApply (the load-path applier) — SORTED by type path
     * name for deterministic save files + deterministic per-entity hydration order. A Produce without a
     * HydrationApply is a misconfig (caught by a registration-time ensure) and is excluded here, so it fails
     * loud rather than silently corrupting the save.
     */
    static auto
    Get_SaveHandlerTypes() -> TArray<const UScriptStruct*>;

    /**
     * Register a single catch-all handler consulted by Resolve() when no per-type handler matches.
     * Used by runtime-typed features (e.g. dynamic fragments) whose payload UScriptStruct is not
     * known at compile time, so per-type registration is impossible. The fallback only ever sees
     * types that nobody registered explicitly — registered types always win via Find().
     */
    static auto
    RegisterFallback(
        FHandler InHandler) -> void;

    static auto
    Find(
        const UScriptStruct* InType) -> const FHandler*;

    /** Find() the per-type handler, else the registered fallback (or nullptr if neither exists). */
    static auto
    Resolve(
        const UScriptStruct* InType) -> const FHandler*;

private:
    static auto
    ResolvePending() -> void;

    struct FLazyEntry
    {
        FTypeResolver TypeResolver;
        FHandler Handler;
    };

    static TMap<const UScriptStruct*, FHandler> _Handlers;
    static TArray<FLazyEntry> _PendingHandlers;
    static TOptional<FHandler> _Fallback;
};

// --------------------------------------------------------------------------------------------------------------------
