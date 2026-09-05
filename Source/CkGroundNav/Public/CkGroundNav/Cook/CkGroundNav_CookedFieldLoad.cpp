#include "CkGroundNav_CookedFieldLoad.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkGroundNav/Cook/CkGroundNav_CookedFieldIndex.h"
#include "CkGroundNav/Field/CkGroundNav_FieldSerialize.h"

#include <Engine/Level.h>
#include <Engine/World.h>
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

    auto
        Find_CookedFieldIndex(
            UWorld* InWorld,
            FName   InCookKey)
        -> const UCk_GroundNav_CookedFieldIndex_UE*
    {
        // None is not a key. Skipped rather than looked up and answered null.
        if (InCookKey.IsNone())
        { return nullptr; }

        const auto LevelPackage = Get_LevelPackageKey(InWorld);

        if (LevelPackage.IsNone())
        { return nullptr; }

        const auto IndexPath = Get_CookedIndexAssetPath(
            kCookedDataRootPath, LevelPackage.ToString(), InCookKey);

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
            FCk_GroundNav_Field&                     OutField)
        -> ECk_GroundNav_CookStatus
    {
        // The asset sitting at the convention path is not necessarily the one that path names: a
        // cooked asset is reached by PATH and by nothing else, so an index moved, renamed or copied
        // from another level would answer this lookup while describing somebody else's ground. It
        // says which level and which volume it was written for, and that is what is checked.
        if (InIndex.Get_LevelPackage() != InLevelPackage || InIndex.Get_CookKey() != InCookKey)
        { return ECk_GroundNav_CookStatus::StaleCook; }

        // Refused before a single tile is resolved: the index carries every one of these so that a
        // whole cooked field can be judged without paying to load the tiles it names.
        if (NOT InIndex.Get_IsCompatibleWith(kFieldBlobFormatVersion))
        { return ECk_GroundNav_CookStatus::StaleCook; }

        if (InIndex.Get_Fingerprint() != InInputFingerprint)
        { return ECk_GroundNav_CookStatus::StaleCook; }

        const auto LatticeKey = Get_CookedLatticeKey(InParams);

        if (NOT (InIndex.Get_LatticeKey() == LatticeKey))
        { return ECk_GroundNav_CookStatus::StaleCook; }

        // Composed into a field of its own and moved into the caller's only once every tile has held,
        // exactly as the whole-field reader does it: a caller handed back a half-loaded field has
        // nothing to fall back to.
        auto Field = FCk_GroundNav_Field{};

        Field._Params = InParams;
        Field._Tiles.SetNum(InParams.Get_TileCount());

        for (auto TileIndex = 0; TileIndex < Field._Tiles.Num(); ++TileIndex)
        { Field._Tiles[TileIndex]._Coord = Get_TileCoord(InParams._Divisions, TileIndex); }

        const auto& CookedTileRefs = InIndex.Get_Tiles();

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

            if (CookedTile->Get_Fingerprint() != InIndex.Get_Fingerprint())
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
}

// --------------------------------------------------------------------------------------------------------------------
