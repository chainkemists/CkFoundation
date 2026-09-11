#include "CkGroundNav/Search/CkGroundNav_FilterCompile.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkGroundNav/Bake/CkGroundNav_Plates.h"
#include "CkGroundNav/Query/CkGroundNav_Query_Reachability.h"

#include "CkNavigation/Nav/CkNav_Fragment_Data.h"
#include "CkNavigation/NavSurface/CkNavSurface_AreaPolicy.h"
#include "CkNavigation/NavSurface/CkNavFilterDefinition_Registry.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::filter_compile_private
{
    /**
     * What one compiled answer is keyed by.
     *
     * The field is identified by a WEAK handle and its epoch together: a published field is immutable,
     * so a handle that still pins and an epoch that still matches is the very snapshot the tables were
     * compiled against. The weak handle is what makes the first half of that sentence true - fields are
     * shared and are freed on rebuild, repair, derive and world teardown, and a raw address would let a
     * recycled allocation carrying a coinciding epoch answer for a field that no longer exists. The
     * overlay reduces to its excluded tags, which is the only field it has, sorted so two callers naming
     * the same set in two orders hit one entry.
     */
    struct FCompiledFilterCacheKey
    {
        TWeakPtr<const FCk_GroundNav_Field> _Field;

        int64 _EpochValue = 0;

        FGameplayTag _FilterTag;

        FString _DefinitionIdentity;

        TArray<FGameplayTag> _OverlayExcludedTags;
    };

    /**
     * The compiled tables, most recently used first.
     *
     * Bounded and tiny on purpose: a route plan issues many queries per frame under a HANDFUL of
     * distinct filters, so the working set is the number of filters in play and not the number of
     * queries. Sixteen holds every filter a frame realistically names; past that the least recently
     * used one is dropped, which costs one field pass to rebuild and never unbounded memory.
     *
     * GAME-THREAD STATE. The only capability contracted to run off the game thread is
     * _BoundarySegments (CkNavSurface_ProviderTable.h:32-36), and a boundary query carries no filter.
     */
    constexpr auto CompiledFilterCacheCapacity = 16;

    auto Get_CompiledFilterCache() -> TArray<TPair<FCompiledFilterCacheKey, FCk_GroundNav_CompiledFilterTables>>&
    {
        static auto Cache = TArray<TPair<FCompiledFilterCacheKey, FCk_GroundNav_CompiledFilterTables>>{};
        return Cache;
    }

    auto Make_CompiledFilterCacheKey(
        const FCk_GroundNav_FieldPtr&     InField,
        const FGameplayTag&               InFilterTag,
        const FString&                    InDefinitionIdentity,
        const FCk_Nav_QueryFilterOverlay& InOverlay) -> FCompiledFilterCacheKey
    {
        auto Key = FCompiledFilterCacheKey{};
        Key._Field = InField;
        Key._EpochValue = InField->_Epoch._Value;
        Key._FilterTag = InFilterTag;
        Key._DefinitionIdentity = InDefinitionIdentity;
        Key._OverlayExcludedTags = InOverlay.Get_ExcludedAreaTags();

        Key._OverlayExcludedTags.Sort([](const FGameplayTag& InLeft, const FGameplayTag& InRight)
        {
            return InLeft.GetTagName().LexicalLess(InRight.GetTagName());
        });

        return Key;
    }

    /** Two keys name the same compiled answer only if BOTH still pin the same field. A key whose field
     *  has been freed names nothing and can never match, which is what retires an entry. */
    auto Get_KeysMatch(
        const FCompiledFilterCacheKey& InLeft,
        const FCompiledFilterCacheKey& InRight) -> bool
    {
        const auto LeftField = InLeft._Field.Pin();
        const auto RightField = InRight._Field.Pin();

        return LeftField.IsValid() && LeftField == RightField &&
               InLeft._EpochValue == InRight._EpochValue &&
               InLeft._FilterTag == InRight._FilterTag &&
               InLeft._DefinitionIdentity == InRight._DefinitionIdentity &&
               InLeft._OverlayExcludedTags == InRight._OverlayExcludedTags;
    }

    auto Get_DefinitionIdentity(
        const FCk_NavFilter_Definition& InDefinition) -> FString
    {
        auto Parts = TArray<FString>{};

        auto RequiredTags = TArray<FGameplayTag>{};
        InDefinition.Get_RequiredAreaTags().GetGameplayTagArray(RequiredTags);
        RequiredTags.Sort([](const FGameplayTag& InLeft, const FGameplayTag& InRight)
        {
            return InLeft.GetTagName().LexicalLess(InRight.GetTagName());
        });
        for (const auto& Tag : RequiredTags)
        { Parts.Emplace(FString::Printf(TEXT("R:%s"), *Tag.ToString())); }

        auto ExcludedTags = TArray<FGameplayTag>{};
        InDefinition.Get_ExcludedAreaTags().GetGameplayTagArray(ExcludedTags);
        ExcludedTags.Sort([](const FGameplayTag& InLeft, const FGameplayTag& InRight)
        {
            return InLeft.GetTagName().LexicalLess(InRight.GetTagName());
        });
        for (const auto& Tag : ExcludedTags)
        { Parts.Emplace(FString::Printf(TEXT("E:%s"), *Tag.ToString())); }

        auto CostEntries = TArray<TPair<FGameplayTag, float>>{};
        CostEntries.Reserve(InDefinition.Get_AreaCostMultipliers().Num());
        for (const auto& Entry : InDefinition.Get_AreaCostMultipliers())
        { CostEntries.Emplace(Entry.Key, Entry.Value); }
        CostEntries.Sort([](const auto& InLeft, const auto& InRight)
        {
            return InLeft.Key.GetTagName().LexicalLess(InRight.Key.GetTagName());
        });
        for (const auto& Entry : CostEntries)
        {
            uint32 CostBits = 0;
            FMemory::Memcpy(&CostBits, &Entry.Value, sizeof(CostBits));
            Parts.Emplace(FString::Printf(
                TEXT("C:%s:%08x"), *Entry.Key.ToString(), CostBits));
        }

        return FString::Join(Parts, TEXT("|"));
    }

    /**
     * The pass over the field: every plate of every tile, matched against the definition's three
     * rules and the overlay's exclusions, in ONE walk.
     *
     * Exclusion and requirement are matched EXACTLY, which is what Recast does - its exclusion
     * resolves one tag to one UNavArea, and its requirement is compared with HasTagExact
     * (CkNavSurface_RecastAdapter.cpp DoApplyDefinition). A requirement refuses a plate that carries
     * SOME area the requirement does not name, and leaves UNMARKED ground alone: Recast expresses a
     * requirement by excluding every REGISTERED area outside it, and plain navmesh is not one of
     * those, so a required set that refused bare ground would refuse what Recast admits.
     */
    auto Do_CompileFilterTables(
        const FCk_GroundNav_Field&        InField,
        const FCk_NavFilter_Definition&   InDefinition,
        const FCk_Nav_QueryFilterOverlay& InOverlay) -> FCk_GroundNav_CompiledFilterTables
    {
        auto Tables = FCk_GroundNav_CompiledFilterTables{};

        auto ExcludedTags = InDefinition.Get_ExcludedAreaTags();

        for (const auto& OverlayTag : InOverlay.Get_ExcludedAreaTags())
        { ExcludedTags.AddTag(OverlayTag); }

        const auto RequiredTags = InDefinition.Get_RequiredAreaTags();
        const auto CostMultipliers = InDefinition.Get_AreaCostMultipliers();

        // Nothing to say about any plate, so nothing is worth a pass over the field.
        if (ExcludedTags.IsEmpty() && RequiredTags.IsEmpty() && CostMultipliers.IsEmpty())
        { return Tables; }

        for (auto TileIndex = 0; TileIndex < InField._Tiles.Num(); ++TileIndex)
        {
            const auto& PlateField = InField._Tiles[TileIndex]._Plates;

            for (auto PlateIndex = 0; PlateIndex < PlateField._Plates.Num(); ++PlateIndex)
            {
                const auto FlatPlate = Get_FlatPlateIndex(InField, TileIndex, PlateIndex);

                if (FlatPlate == INDEX_NONE)
                { continue; }

                const auto& PlateTags = PlateField.Get_AreaPolicy(PlateField._Plates[PlateIndex]._AreaPolicyIndex);

                if (PlateTags.IsEmpty())
                { continue; }

                if (PlateTags.HasAnyExact(ExcludedTags))
                {
                    Tables._Denied.Add(FlatPlate);
                    continue;
                }

                if (NOT RequiredTags.IsEmpty() && NOT PlateTags.HasAnyExact(RequiredTags))
                {
                    Tables._Denied.Add(FlatPlate);
                    continue;
                }

                for (const auto& CostEntry : CostMultipliers)
                {
                    if (NOT PlateTags.HasTagExact(CostEntry.Key))
                    { continue; }

                    auto& Multiplier = Tables._Multipliers.FindOrAdd(FlatPlate);
                    Multiplier = FMath::Max(Multiplier, CostEntry.Value);
                }
            }
        }

        return Tables;
    }

    template <typename TTagRange>
    auto Do_AreAreaTagsRegistered(
        const TTagRange& InAreaTags) -> bool
    {
        for (const auto& AreaTag : InAreaTags)
        {
            const auto AreaPolicyIsRegistered = ck::nav_surface::TryGet_AreaPolicy(AreaTag).IsSet();
            CK_ENSURE_IF_NOT(AreaPolicyIsRegistered,
                TEXT("GroundNav rejected area policy tag [{}] because no neutral area policy is registered"), AreaTag)
            {}

            if (NOT AreaPolicyIsRegistered)
            { return false; }
        }

        return true;
    }

    auto Do_AreDefinitionAreaTagsRegistered(
        const FCk_NavFilter_Definition& InDefinition) -> bool
    {
        if (NOT Do_AreAreaTagsRegistered(InDefinition.Get_RequiredAreaTags()) ||
            NOT Do_AreAreaTagsRegistered(InDefinition.Get_ExcludedAreaTags()))
        { return false; }

        for (const auto& CostEntry : InDefinition.Get_AreaCostMultipliers())
        {
            const auto AreaPolicyIsRegistered = ck::nav_surface::TryGet_AreaPolicy(CostEntry.Key).IsSet();
            CK_ENSURE_IF_NOT(AreaPolicyIsRegistered,
                TEXT("GroundNav rejected area policy tag [{}] because no neutral area policy is registered"),
                CostEntry.Key)
            {}

            if (NOT AreaPolicyIsRegistered)
            { return false; }
        }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        TryGet_CompiledFilterTables(
            const FCk_GroundNav_FieldPtr&     InField,
            const FGameplayTag&               InFilterTag,
            const FCk_Nav_QueryFilterOverlay& InOverlay)
        -> const FCk_GroundNav_CompiledFilterTables*
    {
        using namespace filter_compile_private;

        static const auto NoTables = FCk_GroundNav_CompiledFilterTables{};

        const auto Definition = ck::nav_surface::TryGet_FilterDefinition(InFilterTag);
        const auto NamedFilterResolved = NOT InFilterTag.IsValid() || Definition.IsSet();
        CK_ENSURE_IF_NOT(NamedFilterResolved,
            TEXT("GroundNav rejected named query filter [{}] because no valid definition resolved"), InFilterTag)
        {}

        if (NOT NamedFilterResolved)
        { return nullptr; }

        const auto DefinitionAreaTagsAreRegistered =
            NOT Definition.IsSet() || Do_AreDefinitionAreaTagsRegistered(Definition.GetValue());
        const auto OverlayAreaTagsAreRegistered = Do_AreAreaTagsRegistered(InOverlay.Get_ExcludedAreaTags());
        const auto AreaTagsAreRegistered = DefinitionAreaTagsAreRegistered && OverlayAreaTagsAreRegistered;
        if (NOT AreaTagsAreRegistered)
        { return nullptr; }

        // A field the caller no longer holds has no plates to compile against. It cannot share the
        // empty-table success answer: callers must not turn invalid field state into a runnable query.
        if (NOT InField.IsValid())
        { return nullptr; }

        // A query that names no filter and no overlay can want nothing, and answering it from the
        // cache would evict a real entry to store an empty one.
        if (NOT InFilterTag.IsValid() && InOverlay.Get_ExcludedAreaTags().IsEmpty())
        { return &NoTables; }

        auto& Cache = Get_CompiledFilterCache();

        // Entries whose field has been freed are retired on the way past rather than by a sweep of
        // their own: the cache is only ever walked here, so this IS every time one could be noticed.
        Cache.RemoveAll([](const auto& InEntry) -> bool
        {
            return NOT InEntry.Key._Field.IsValid();
        });

        const auto DefinitionIdentity = Definition.IsSet()
            ? Get_DefinitionIdentity(Definition.GetValue())
            : FString{};
        const auto Key = Make_CompiledFilterCacheKey(InField, InFilterTag, DefinitionIdentity, InOverlay);

        const auto Found = Cache.IndexOfByPredicate([&](const auto& InEntry)
        {
            return Get_KeysMatch(InEntry.Key, Key);
        });

        if (Found != INDEX_NONE)
        {
            // Most recently used first, so the entry dropped below is the one nothing has asked for.
            if (Found != 0)
            {
                auto Entry = MoveTemp(Cache[Found]);
                Cache.RemoveAt(Found);
                Cache.Insert(MoveTemp(Entry), 0);
            }

            return &Cache[0].Value;
        }

        Cache.Insert(TPair<FCompiledFilterCacheKey, FCk_GroundNav_CompiledFilterTables>{
            Key, Do_CompileFilterTables(
                *InField,
                Definition.IsSet() ? Definition.GetValue() : FCk_NavFilter_Definition{},
                InOverlay)}, 0);

        if (Cache.Num() > CompiledFilterCacheCapacity)
        { Cache.SetNum(CompiledFilterCacheCapacity); }

        return &Cache[0].Value;
    }
}

// --------------------------------------------------------------------------------------------------------------------
