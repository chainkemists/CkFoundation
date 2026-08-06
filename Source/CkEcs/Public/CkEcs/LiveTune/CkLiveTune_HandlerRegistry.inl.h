#pragma once

// Out-of-line bodies for FCk_LiveTuneHandlerRegistry's template registration methods — kept out of the
// registry header so they instantiate in the registering .cpp, where T_Params is a complete type.
// Include both there.

#include "CkEcs/LiveTune/CkLiveTune_HandlerRegistry.h"

#include "CkCore/Ensure/CkEnsure.h"

#if WITH_EDITOR

template <typename T_Params, typename T_Fragment>
auto
    FCk_LiveTuneHandlerRegistry::
    Register(TArgs<T_Params> InArgs)
    -> void
{
    const auto HasCustomApply = static_cast<bool>(InArgs.Apply);

    auto Handler = FHandler{};
    Handler.Kind = ECk_LiveTune_ApplyKind::Direct;

    // Auto resolves against the re-apply, not the author's optimism: a fragment swap is cheap enough to
    // preview on every drag-frame, anything routed through a feature's own request is assumed not to be
    // until that feature says so explicitly.
    Handler.ScrubPolicy = InArgs.ScrubPolicy != ECk_LiveTune_ScrubPolicy::Auto
        ? InArgs.ScrubPolicy
        : (HasCustomApply ? ECk_LiveTune_ScrubPolicy::OnCommit : ECk_LiveTune_ScrubPolicy::DuringScrub);

    // Only the default Replace path guarantees the params fragment is on the entity, so it is the only one
    // Link can validate against.
    if (NOT HasCustomApply)
    {
        Handler.HasFragment = [](const FCk_Handle& InEntity) -> bool
        {
            return InEntity.Has<T_Fragment>();
        };
    }

    Handler.Apply = [Apply = MoveTemp(InArgs.Apply), PostApply = MoveTemp(InArgs.PostApply)]
        (FCk_Handle& InEntity, const FInstancedStruct& InFreshParams) -> void
    {
        if (Apply)
        {
            Apply(InEntity, InFreshParams.Get<T_Params>());
        }
        else
        {
            const auto EntityHasParams = InEntity.Has<T_Fragment>();
            CK_ENSURE_IF_NOT(EntityHasParams,
                TEXT("LiveTune: Entity [{}] no longer carries params fragment [{}] — cannot re-apply"),
                InEntity, T_Params::StaticStruct()->GetName())
            {}
            if (NOT EntityHasParams)
            { return; }

            // T_Fragment{params} covers both forms: a copy when the fragment IS the params type, and the
            // wrapper's CK_DEFINE_CONSTRUCTORS when it holds them.
            InEntity.Replace<T_Fragment>(T_Fragment{InFreshParams.Get<T_Params>()});
        }

        if (PostApply)
        { PostApply(InEntity); }
    };

    RegisterLazy([]() -> UScriptStruct* { return T_Params::StaticStruct(); }, MoveTemp(Handler));
}

template <typename T_Params>
auto
    FCk_LiveTuneHandlerRegistry::
    Register_ViaRebuild(TArgs_ViaRebuild<T_Params> InArgs)
    -> void
{
    auto Handler = FHandler{};
    Handler.Kind = ECk_LiveTune_ApplyKind::Rebuild;

    // Not configurable: a destroy/re-Add/hydrate cycle per drag-frame is precisely the storm the scrub
    // policy exists to prevent, so a rebuild always waits for the committed value.
    Handler.ScrubPolicy = ECk_LiveTune_ScrubPolicy::OnCommit;

    Handler.RebuildScope = InArgs.Scope;
    Handler.ReAdd = [ReAdd = MoveTemp(InArgs.ReAdd.Value)](FCk_Handle& InOwner, const FInstancedStruct& InFreshParams) -> FCk_Handle
    {
        return ReAdd(InOwner, InFreshParams.Get<T_Params>());
    };
    if (InArgs.Capture)
    {
        Handler.CaptureOverride = [Capture = MoveTemp(InArgs.Capture)](FCk_Handle& InLinkedEntity, const FInstancedStruct& InFreshParams) -> TOptional<FInstancedStruct>
        {
            return Capture(InLinkedEntity, InFreshParams.Get<T_Params>());
        };
    }

    RegisterLazy([]() -> UScriptStruct* { return T_Params::StaticStruct(); }, MoveTemp(Handler));
}

#endif
