#include "CkAssetReferenceProvider.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetReferenceProviderRegistry::
    Get()
    -> FCk_AssetReferenceProviderRegistry&
{
    static auto Registry = FCk_AssetReferenceProviderRegistry{};
    return Registry;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetReferenceProviderRegistry::
    Request_Register(
        FName InSourceId,
        FCk_Delegate_AssetReference_Query InQuery)
    -> void
{
    const auto OnGameThread = IsInGameThread();
    CK_ENSURE_IF_NOT(OnGameThread,
        TEXT("Asset-reference provider [{}] registered off the game thread. The registry is unsynchronized by "
             "design because every known caller is game-thread; registering here would race a scan."), InSourceId)
    { return; }

    const auto SourceIdIsValid = NOT InSourceId.IsNone();
    CK_ENSURE_IF_NOT(SourceIdIsValid,
        TEXT("Asset-reference provider registered with no source id. The id is what a status line prints and what "
             "unregister matches on, so an unnamed provider could never be removed."))
    { return; }

    const auto QueryIsBound = InQuery.IsBound();
    CK_ENSURE_IF_NOT(QueryIsBound,
        TEXT("Asset-reference provider [{}] registered with an unbound query. An entry that always answers 'no "
             "referencers' is indistinguishable from one that answers correctly, which is the exact confusion this "
             "registry exists to remove."), InSourceId)
    { return; }

    if (auto* Existing = TryFind_Provider(InSourceId);
        Existing != nullptr)
    {
        // Replace rather than add. A module that re-registers after a reload must end up with ONE entry: two
        // providers under one id would double every count and could disagree about the same asset.
        ck::core::Verbose(TEXT("Replacing existing asset-reference provider [{}]"), InSourceId);
        Existing->Query = MoveTemp(InQuery);
        return;
    }

    _Providers.Add(FProvider{InSourceId, MoveTemp(InQuery)});

    // Sorted on insert so every projection over the array is deterministic without re-sorting. The set is single-digit
    // and only changes at module load, so the cost is not worth measuring.
    _Providers.Sort([](const FProvider& InLhs, const FProvider& InRhs) -> bool
    {
        return InLhs.SourceId.LexicalLess(InRhs.SourceId);
    });

    ck::core::Log(TEXT("Registered asset-reference provider [{}] — {} provider(s) now answer for references the package "
                   "graph cannot see"), InSourceId, _Providers.Num());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetReferenceProviderRegistry::
    Request_Unregister(
        FName InSourceId)
    -> void
{
    const auto OnGameThread = IsInGameThread();
    CK_ENSURE_IF_NOT(OnGameThread,
        TEXT("Asset-reference provider [{}] unregistered off the game thread."), InSourceId)
    { return; }

    const auto RemovedCount = _Providers.RemoveAll([InSourceId](const FProvider& InProvider) -> bool
    {
        return InProvider.SourceId == InSourceId;
    });

    if (RemovedCount == 0)
    { return; }

    ck::core::Log(TEXT("Unregistered asset-reference provider [{}]"), InSourceId);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetReferenceProviderRegistry::
    Get_ExternalReferences(
        const FSoftObjectPath& InAsset) const
    -> TArray<FCk_AssetReference_External>
{
    auto Results = TArray<FCk_AssetReference_External>{};

    if (InAsset.IsNull())
    { return Results; }

    for (const auto& Provider : _Providers)
    {
        auto Referencers = Provider.Query.Execute(InAsset);

        // A provider that knows of none is omitted rather than returned empty: the caller is asking "who points at
        // this", and an entry answering "nobody" is not an answer to that question.
        if (Referencers.IsEmpty())
        { continue; }

        Results.Add(FCk_AssetReference_External{Provider.SourceId, MoveTemp(Referencers)});
    }

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetReferenceProviderRegistry::
    Get_HasAnyProvider() const
    -> bool
{
    return NOT _Providers.IsEmpty();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetReferenceProviderRegistry::
    TryFind_Provider(
        FName InSourceId)
    -> FProvider*
{
    return _Providers.FindByPredicate([InSourceId](const FProvider& InProvider) -> bool
    {
        return InProvider.SourceId == InSourceId;
    });
}

// --------------------------------------------------------------------------------------------------------------------
