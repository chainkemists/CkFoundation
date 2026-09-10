#include "CkGroundNav_CookedFieldLoad.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkGroundNav/Cook/CkGroundNav_CookedFieldIndex.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedSourceManifest.h"
#include "CkGroundNav/Field/CkGroundNav_FieldSerialize.h"
#include "CkGroundNav/Field/CkGroundNav_TileBake.h"

#include <Engine/Level.h>
#include <Engine/World.h>
#include <Misc/PackageName.h>
#include <UObject/Package.h>
#include <UObject/UObjectGlobals.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Get_CookedLatticeKey(
            const FCk_GroundNav_FieldParams& InParams)
        -> FCk_GroundNav_CookedLatticeKey
    {
        auto Key = FCk_GroundNav_CookedLatticeKey{};

        Key.Set_OriginXY(InParams._OriginXY);
        Key.Set_Divisions(InParams._Divisions);
        Key.Set_MinZUu(InParams._MinZUu);
        Key.Set_MaxZUu(InParams._MaxZUu);
        Key.Set_TileSizeUu(InParams._Config.Get_TileSizeUu());
        Key.Set_CellSizeUu(InParams._Config.Get_CellSizeUu());
        Key.Set_CellHeightUu(InParams._Config.Get_CellHeightUu());

        return Key;
    }

    auto
    Get_LevelPackageKey(
        UWorld* InWorld)
        -> FName
    {
        if (ck::Is_NOT_Valid(InWorld) || ck::Is_NOT_Valid(InWorld->PersistentLevel))
        { return NAME_None; }

        return Get_PackageLookupKey(InWorld->PersistentLevel->GetOutermost()->GetName());
    }

    auto Get_CookedSourceManifestAssetPath(
        const FName InSourceLevelPackage, const FName InCookKey,
        const FCk_GroundNav_DataLayerSelector& InDataLayerSelector, const int32 InPartitionId) -> FString
    {
        if (InSourceLevelPackage.IsNone() || InCookKey.IsNone() || InPartitionId <= 0 ||
            NOT InDataLayerSelector.Get_IsCanonical())
        { return {}; }

        const auto DefaultIndexPath = Get_CookedIndexAssetPath(
            kCookedDataRootPath, InSourceLevelPackage.ToString(), InCookKey, {}, InDataLayerSelector);
        const auto DefaultIndexPackage = FPackageName::ObjectPathToPackageName(DefaultIndexPath);
        const auto Directory = FPackageName::GetLongPackagePath(DefaultIndexPackage);
        const auto AssetName = FString::Printf(TEXT("GroundNavSourceManifest_%s_%d"),
            *InCookKey.ToString(), InPartitionId);
        return FString::Printf(TEXT("%s/%s.%s"), *Directory, *AssetName, *AssetName);
    }

    auto
        Find_CookedFieldIndex(
            UWorld* InWorld,
            FName   InCookKey,
            FGameplayTag InProfileTag,
            FName InSourceLevelPackage,
            const FCk_GroundNav_DataLayerSelector& InDataLayerSelector)
        -> const UCk_GroundNav_CookedFieldIndex_UE*
    {
        // None is not a key. Skipped rather than looked up and answered null.
        if (InCookKey.IsNone() || NOT InDataLayerSelector.Get_IsCanonical())
        { return nullptr; }

        const auto LevelPackage = InSourceLevelPackage.IsNone()
            ? Get_LevelPackageKey(InWorld)
            : Get_PackageLookupKey(InSourceLevelPackage.ToString());

        if (LevelPackage.IsNone())
        { return nullptr; }

        const auto IndexPath = Get_CookedIndexAssetPath(
            kCookedDataRootPath, LevelPackage.ToString(), InCookKey, InProfileTag, InDataLayerSelector);

        // Cook keys and source packages are authored values. Do not hand an object-path parser a
        // malformed value: a bad convention path is simply a cook that cannot be found.
        if (NOT FPackageName::IsValidObjectPath(IndexPath))
        { return nullptr; }

        // LOAD_NoWarn | LOAD_Quiet, the same way CkJolt's cooked mesh shape is reached: a level that
        // was never cooked is an EXPECTED miss whose answer is to bake at runtime, and the engine's
        // default load path emits LogUObjectGlobals "Failed to find object" and LogStreaming
        // "SkipPackage" warnings that an automation run captures as failures.
        return LoadObject<UCk_GroundNav_CookedFieldIndex_UE>(
            nullptr, *IndexPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    }

    auto
        Try_LoadCookedField(
            const UCk_GroundNav_CookedFieldIndex_UE& InIndex,
            FName                                    InLevelPackage,
            FName                                    InCookKey,
            const FCk_GroundNav_FieldParams&         InParams,
            uint64                                   InInputFingerprint,
            FCk_GroundNav_Field&                     OutField,
            FGameplayTag                             InProfileTag,
            int32                                    InStreamingVolumeId,
            const FCk_GroundNav_DataLayerSelector&   InDataLayerSelector)
        -> ECk_GroundNav_CookStatus
    {
        // The asset sitting at the convention path is not necessarily the one that path names: a
        // cooked asset is reached by PATH and by nothing else, so an index moved, renamed or copied
        // from another level would answer this lookup while describing somebody else's ground. It
        // says which level and which volume it was written for, and that is what is checked.
        if (InIndex.Get_LevelPackage() != InLevelPackage || InIndex.Get_CookKey() != InCookKey ||
            InIndex.Get_ProfileTag() != InProfileTag)
        { return ECk_GroundNav_CookStatus::StaleCook; }

        // A positive id opts into streamed identity. The old assets carry INDEX_NONE and retain the
        // original cook-key behaviour, but a streamed caller never admits an unscoped manifest.
        if (InStreamingVolumeId == 0 || InIndex.Get_StreamingVolumeId() == 0)
        { return ECk_GroundNav_CookStatus::StaleCook; }

        const auto HasStreamingIdentity = InStreamingVolumeId > 0 || InIndex.Get_StreamingVolumeId() > 0;

        if (HasStreamingIdentity &&
            (InStreamingVolumeId <= 0 || InIndex.Get_StreamingVolumeId() != InStreamingVolumeId))
        { return ECk_GroundNav_CookStatus::StaleCook; }

        // The selector's representation is canonical before it reaches either cooker or loader. A
        // malformed asset is rejected before a tile is resolved, and a value mismatch is a stale
        // bake: it names geometry collected under a different data-layer population.
        if (NOT InDataLayerSelector.Get_IsCanonical() ||
            InIndex.Get_DataLayerNames() != InDataLayerSelector.Get_LayerNames())
        { return ECk_GroundNav_CookStatus::StaleCook; }

        auto IndexSelector = FCk_GroundNav_DataLayerSelector{};

        if (NOT TryMake_DataLayerSelector(InIndex.Get_DataLayerNames(), IndexSelector) ||
            IndexSelector.Get_LayerNames() != InIndex.Get_DataLayerNames())
        { return ECk_GroundNav_CookStatus::StaleCook; }

        // Refused before a single tile is resolved: the index carries every one of these so that a
        // whole cooked field can be judged without paying to load the tiles it names.
        if (NOT InIndex.Get_IsCompatibleWith(kFieldBlobFormatVersion))
        { return ECk_GroundNav_CookStatus::StaleCook; }

        if (InIndex.Get_Fingerprint() != InInputFingerprint)
        { return ECk_GroundNav_CookStatus::StaleCook; }

        // This is a boundary that accepts data from assets. Reject a malformed lattice before using
        // its dimensions for either allocation or tile indexing; an invalid params object is no more
        // a field than a corrupt index is.
        if (NOT InParams.Get_IsValid())
        { return ECk_GroundNav_CookStatus::StaleCook; }

        const auto ExpectedTileCount = int64{InParams._Divisions.X} * int64{InParams._Divisions.Y};

        if (ExpectedTileCount <= 0 || ExpectedTileCount > MAX_int32)
        { return ECk_GroundNav_CookStatus::StaleCook; }

        const auto LatticeKey = Get_CookedLatticeKey(InParams);

        if (NOT (InIndex.Get_LatticeKey() == LatticeKey))
        { return ECk_GroundNav_CookStatus::StaleCook; }

        const auto& CookedTileRefs = InIndex.Get_Tiles();

        // A field has exactly one asset reference per lattice tile. Check this before allocating or
        // resolving one tile: a malformed index should remain a cheap fallback to runtime baking.
        if (CookedTileRefs.Num() != ExpectedTileCount)
        { return ECk_GroundNav_CookStatus::StaleCook; }

        // Composed into a field of its own and moved into the caller's only once every tile has held,
        // exactly as the whole-field reader does it: a caller handed back a half-loaded field has
        // nothing to fall back to.
        auto Field = FCk_GroundNav_Field{};

        Field._Params = InParams;
        Field._Tiles.SetNum(static_cast<int32>(ExpectedTileCount));

        for (auto TileIndex = 0; TileIndex < Field._Tiles.Num(); ++TileIndex)
        { Field._Tiles[TileIndex]._Coord = Get_TileCoord(InParams._Divisions, TileIndex); }

        for (auto SlotIndex = 0; SlotIndex < CookedTileRefs.Num(); ++SlotIndex)
        {
            const auto* CookedTile = CookedTileRefs[SlotIndex].LoadSynchronous();

            // A reference that resolves to nothing is a tile the cook wrote and something has since
            // deleted or renamed. The index is describing a field that is not there.
            if (ck::Is_NOT_Valid(CookedTile))
            { return ECk_GroundNav_CookStatus::StaleCook; }

            // What the ASSET claims about itself, judged HERE. The blob's own header answers for the
            // bytes below it - magic, version, truncation, tags, the lattice the header carries - and
            // the serializer is what asks those. These four are the tile asset's claims ABOUT that
            // blob, and until now nothing compared them with the index that lists it: a tile written
            // under another format, produced on another lattice, cooked from other inputs, or filed
            // in a slot that is not its own coord would load its bytes perfectly and place them over
            // ground it was never baked from.
            if (NOT CookedTile->Get_IsCompatibleWith(kFieldBlobFormatVersion))
            { return ECk_GroundNav_CookStatus::StaleCook; }

            if (NOT (CookedTile->Get_LatticeKey() == InIndex.Get_LatticeKey()))
            { return ECk_GroundNav_CookStatus::StaleCook; }

            const auto SlotCoord = Get_TileCoord(InParams._Divisions, SlotIndex);

            if (CookedTile->Get_TileCoord() != FIntPoint{SlotCoord._X, SlotCoord._Y})
            { return ECk_GroundNav_CookStatus::StaleCook; }

            if (HasStreamingIdentity &&
                CookedTile->Get_StreamingVolumeId() != InIndex.Get_StreamingVolumeId())
            { return ECk_GroundNav_CookStatus::StaleCook; }

            const auto ExpectedBounds = Get_TileBounds(
                InParams.Get_TileBakeParams(SlotCoord, FCk_GroundNav_Epoch{}));

            if (HasStreamingIdentity &&
                (CookedTile->Get_WorldBounds().IsValid == 0 ||
                 NOT (CookedTile->Get_WorldBounds() == ExpectedBounds)))
            { return ECk_GroundNav_CookStatus::StaleCook; }

            if (CookedTile->Get_DataLayerNames() != InIndex.Get_DataLayerNames())
            { return ECk_GroundNav_CookStatus::StaleCook; }

            if (CookedTile->Get_ContentHash() == 0 ||
                CookedTile->Get_ContentHash() != Get_CookedTileContentHash(CookedTile->Get_Blob()))
            { return ECk_GroundNav_CookStatus::StaleCook; }

            if (CookedTile->Get_Fingerprint() != InIndex.Get_Fingerprint())
            { return ECk_GroundNav_CookStatus::StaleCook; }

            if (CookedTile->Get_ProfileTag() != InProfileTag)
            { return ECk_GroundNav_CookStatus::StaleCook; }

            // Read DEFERRED: the derives are whole-field, so composing per tile would run them once
            // per tile and throw away every answer but the last. The one composition below is that
            // last run, and it is the only one worth paying for.
            if (Read_TileInto(CookedTile->Get_Blob(), Field, ECk_GroundNav_ComposeOnLoad::Deferred) !=
                ECk_GroundNav_LoadStatus::Loaded)
            { return ECk_GroundNav_CookStatus::StaleCook; }
        }

        Compose_LoadedField(Field);

        // _OpenBodies is left as the empty array it was constructed with. A per-tile blob carries no
        // open-body report, and the closure check belongs to the run that read the meshes - which was
        // the cook's, and is the cooker's to report.
        OutField = MoveTemp(Field);

        return ECk_GroundNav_CookStatus::Cooked;
    }

    auto
        Try_LoadCookedSourceManifest(
            const UCk_GroundNav_CookedSourceManifest_UE& InManifest,
            const FCk_GroundNav_StreamFieldBundle&       InTemplateBundle,
            uint64                                       InDefaultFingerprint,
            const TMap<FGameplayTag, uint64>&            InVariantFingerprints,
            int32                                        InStreamingVolumeId,
            int32                                        InPartitionId,
            const FCk_GroundNav_DataLayerSelector&       InDataLayerSelector,
            TArray<FCk_GroundNav_StreamTileTransition>&  OutTransitions)
        -> ECk_GroundNav_CookStatus
    {
        const auto& DefaultField = InTemplateBundle._DefaultField;

        if (InStreamingVolumeId <= 0 || InPartitionId <= 0 ||
            InManifest.Get_StreamingVolumeId() != InStreamingVolumeId ||
            InManifest.Get_PartitionId() != InPartitionId ||
            NOT InDataLayerSelector.Get_IsCanonical() ||
            InManifest.Get_DataLayerNames() != InDataLayerSelector.Get_LayerNames() ||
            NOT InManifest.Get_IsCompatibleWith(kFieldBlobFormatVersion) ||
            NOT DefaultField._Params.Get_IsValid() ||
            DefaultField._Tiles.Num() != DefaultField._Params.Get_TileCount() ||
            NOT (InManifest.Get_LatticeKey() == Get_CookedLatticeKey(DefaultField._Params)))
        {
            return ECk_GroundNav_CookStatus::StaleCook;
        }

        auto ManifestSelector = FCk_GroundNav_DataLayerSelector{};
        if (NOT TryMake_DataLayerSelector(InManifest.Get_DataLayerNames(), ManifestSelector) ||
            ManifestSelector.Get_LayerNames() != InManifest.Get_DataLayerNames())
        {
            return ECk_GroundNav_CookStatus::StaleCook;
        }

        if (InVariantFingerprints.Num() != InTemplateBundle._VariantFields.Num() ||
            InManifest.Get_Profiles().Num() != InTemplateBundle._VariantFields.Num() + 1 ||
            InManifest.Get_TileCoords().IsEmpty())
        {
            return ECk_GroundNav_CookStatus::StaleCook;
        }

        auto PreviousCoord = FIntPoint{};
        auto HasPreviousCoord = false;
        for (const auto& Coord : InManifest.Get_TileCoords())
        {
            const auto IsStrictlyAfterPrevious = !HasPreviousCoord ||
                Coord.Y > PreviousCoord.Y || (Coord.Y == PreviousCoord.Y && Coord.X > PreviousCoord.X);
            if (NOT IsStrictlyAfterPrevious || Coord.X < 0 || Coord.Y < 0 ||
                Coord.X >= DefaultField._Params._Divisions.X || Coord.Y >= DefaultField._Params._Divisions.Y)
            {
                return ECk_GroundNav_CookStatus::StaleCook;
            }
            PreviousCoord = Coord;
            HasPreviousCoord = true;
        }

        auto ProfilesByTag = TMap<FGameplayTag, const FCk_GroundNav_CookedSourceProfile_UE*>{};
        auto DefaultProfileCount = 0;
        for (const auto& Profile : InManifest.Get_Profiles())
        {
            const auto ProfileTag = Profile.Get_ProfileTag();
            if (!ProfileTag.IsValid())
            {
                ++DefaultProfileCount;
            }
            if (ProfilesByTag.Contains(ProfileTag) || Profile.Get_Tiles().Num() != InManifest.Get_TileCoords().Num())
            {
                return ECk_GroundNav_CookStatus::StaleCook;
            }
            ProfilesByTag.Add(ProfileTag, &Profile);
        }

        const auto* DefaultProfile = ProfilesByTag.FindRef(FGameplayTag{});
        if (DefaultProfileCount != 1 || DefaultProfile == nullptr ||
            DefaultProfile->Get_Fingerprint() != InDefaultFingerprint)
        {
            return ECk_GroundNav_CookStatus::StaleCook;
        }

        for (const auto& Variant : InTemplateBundle._VariantFields)
        {
            const auto* ExpectedFingerprint = InVariantFingerprints.Find(Variant.Key);
            const auto* Profile = ProfilesByTag.FindRef(Variant.Key);
            if (!Variant.Key.IsValid() || ExpectedFingerprint == nullptr || Profile == nullptr ||
                Profile->Get_Fingerprint() != *ExpectedFingerprint ||
                NOT Variant.Value._Params.Get_IsValid() ||
                Variant.Value._Tiles.Num() != Variant.Value._Params.Get_TileCount() ||
                NOT (Get_CookedLatticeKey(Variant.Value._Params) == InManifest.Get_LatticeKey()))
            {
                return ECk_GroundNav_CookStatus::StaleCook;
            }
        }

        auto ValidateTile = [&](const UCk_GroundNav_CookedTile_UE& InTile,
                                const FCk_GroundNav_CookedSourceProfile_UE& InProfile,
                                const FCk_GroundNav_Field& InTemplate,
                                const FIntPoint& InCoord) -> bool
        {
            const auto ExpectedBounds = Get_TileBounds(
                InTemplate._Params.Get_TileBakeParams(
                    FCk_GroundNav_TileCoord{InCoord.X, InCoord.Y}, FCk_GroundNav_Epoch{}));
            if (NOT InTile.Get_IsCompatibleWith(kFieldBlobFormatVersion) ||
                InTile.Get_StreamingVolumeId() != InStreamingVolumeId ||
                InTile.Get_TileCoord() != InCoord ||
                NOT (InTile.Get_LatticeKey() == InManifest.Get_LatticeKey()) ||
                InTile.Get_WorldBounds().IsValid == 0 || NOT (InTile.Get_WorldBounds() == ExpectedBounds) ||
                InTile.Get_DataLayerNames() != InManifest.Get_DataLayerNames() ||
                InTile.Get_Fingerprint() != InProfile.Get_Fingerprint() ||
                InTile.Get_ProfileTag() != InProfile.Get_ProfileTag() ||
                InTile.Get_ContentHash() == 0 ||
                InTile.Get_ContentHash() != Get_CookedTileContentHash(InTile.Get_Blob()))
            {
                return false;
            }

            auto Decoded = InTemplate;
            const auto Status = Read_TileInto(InTile.Get_Blob(), Decoded, ECk_GroundNav_ComposeOnLoad::Deferred);
            const auto* DecodedTile = Decoded.Get_Tile(FCk_GroundNav_TileCoord{InCoord.X, InCoord.Y});
            return Status == ECk_GroundNav_LoadStatus::Loaded && DecodedTile != nullptr &&
                   DecodedTile->_Coord == FCk_GroundNav_TileCoord{InCoord.X, InCoord.Y} && DecodedTile->Get_IsBuilt();
        };

        auto Candidate = TArray<FCk_GroundNav_StreamTileTransition>{};
        Candidate.Reserve(InManifest.Get_TileCoords().Num());
        for (auto Index = 0; Index < InManifest.Get_TileCoords().Num(); ++Index)
        {
            const auto Coord = InManifest.Get_TileCoords()[Index];
            const auto* DefaultTile = DefaultProfile->Get_Tiles()[Index].LoadSynchronous();
            if (ck::Is_NOT_Valid(DefaultTile) || NOT ValidateTile(*DefaultTile, *DefaultProfile, DefaultField, Coord))
            {
                return ECk_GroundNav_CookStatus::StaleCook;
            }

            auto Transition = FCk_GroundNav_StreamTileTransition{};
            Transition._TileId = {FCk_GroundNav_VolumeId{InStreamingVolumeId}, {Coord.X, Coord.Y}};
            Transition._Kind = ECk_GroundNav_StreamTileTransitionKind::Replace;
            Transition._DefaultBlob = DefaultTile->Get_Blob();

            for (const auto& Variant : InTemplateBundle._VariantFields)
            {
                const auto* Profile = ProfilesByTag.FindRef(Variant.Key);
                const auto* Tile = Profile == nullptr ? nullptr : Profile->Get_Tiles()[Index].LoadSynchronous();
                if (ck::Is_NOT_Valid(Tile) || NOT ValidateTile(*Tile, *Profile, Variant.Value, Coord))
                {
                    return ECk_GroundNav_CookStatus::StaleCook;
                }
                Transition._VariantBlobs.Add(Variant.Key, Tile->Get_Blob());
            }

            Candidate.Emplace(MoveTemp(Transition));
        }

        OutTransitions = MoveTemp(Candidate);
        return ECk_GroundNav_CookStatus::Cooked;
    }
}

// --------------------------------------------------------------------------------------------------------------------
