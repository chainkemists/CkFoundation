#include "CkGroundNav_FieldStreaming.h"

#include "CkGroundNav/Field/CkGroundNav_FieldLinks.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    namespace field_streaming_private
    {
        struct FDecodedTransition
        {
            FCk_GroundNav_TileCoord _Coord;

            ECk_GroundNav_StreamTileTransitionKind _Kind = ECk_GroundNav_StreamTileTransitionKind::Replace;

            FCk_GroundNav_Tile _Tile;
        };

        auto
            Decode_ReplaceTile(
                const FCk_GroundNav_Field&     InBase,
                const TArray<uint8>&           InBlob,
                const FCk_GroundNav_TileCoord& InCoord,
                FCk_GroundNav_Tile&            OutTile)
            -> ECk_GroundNav_LoadStatus
        {
            auto Decode = InBase;

            for (auto TileIndex = 0; TileIndex < Decode._Tiles.Num(); ++TileIndex)
            {
                auto Unbuilt = FCk_GroundNav_Tile{};
                Unbuilt._Coord = Get_TileCoord(Decode._Params._Divisions, TileIndex);
                Decode._Tiles[TileIndex] = MoveTemp(Unbuilt);
            }

            const auto Status = Read_TileInto(InBlob, Decode, ECk_GroundNav_ComposeOnLoad::Deferred);
            const auto* Tile = Decode.Get_Tile(InCoord);

            if (Status != ECk_GroundNav_LoadStatus::Loaded || Tile == nullptr ||
                Tile->_Coord != InCoord || NOT Tile->Get_IsBuilt())
            {
                return Status == ECk_GroundNav_LoadStatus::Loaded
                    ? ECk_GroundNav_LoadStatus::Corrupt
                    : Status;
            }

            OutTile = *Tile;
            return ECk_GroundNav_LoadStatus::Loaded;
        }

        auto
            Compose_Profile(
                const FCk_GroundNav_Field&        InBase,
                const TArray<FDecodedTransition>& InTransitions,
                FCk_GroundNav_Field&              OutField)
            -> void
        {
            OutField = InBase;

            auto ChangedCoords = TArray<FCk_GroundNav_TileCoord>{};
            ChangedCoords.Reserve(InTransitions.Num());

            for (const auto& Transition : InTransitions)
            {
                const auto TileIndex = Get_TileIndex(OutField._Params._Divisions, Transition._Coord);

                if (Transition._Kind == ECk_GroundNav_StreamTileTransitionKind::Replace)
                {
                    OutField._Tiles[TileIndex] = Transition._Tile;
                }
                else
                {
                    auto Unbuilt = FCk_GroundNav_Tile{};
                    Unbuilt._Coord = Transition._Coord;
                    OutField._Tiles[TileIndex] = MoveTemp(Unbuilt);
                }

                ChangedCoords.Emplace(Transition._Coord);
            }

            DoDerive_SeamPortalsForChangedTiles(OutField, ChangedCoords);
            DoResolve_Links(OutField);
            DoLabel_Reachability(OutField);
        }
    }

    auto
        Compose_StreamTileTransitions(
            const FCk_GroundNav_StreamFieldBundle&              InBaseBundle,
            FCk_GroundNav_VolumeId                               InVolumeId,
            TConstArrayView<FCk_GroundNav_StreamTileTransition> InTransitions,
            FCk_GroundNav_StreamFieldBundle&                    OutBundle)
        -> FCk_GroundNav_StreamCompositionResult
    {
        using namespace field_streaming_private;

        auto Result = FCk_GroundNav_StreamCompositionResult{};
        const auto& Default = InBaseBundle._DefaultField;

        if (NOT InVolumeId.Get_IsStreamingValid())
        {
            Result._Status = ECk_GroundNav_StreamCompositionStatus::InvalidVolumeId;
            return Result;
        }

        if (InTransitions.IsEmpty())
        {
            Result._Status = ECk_GroundNav_StreamCompositionStatus::NoChange;
            return Result;
        }

        if (Default._Tiles.Num() != Default._Params.Get_TileCount())
        {
            Result._Status = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
            return Result;
        }

        for (const auto& Variant : InBaseBundle._VariantFields)
        {
            if (NOT Variant.Key.IsValid() ||
                Variant.Value._Tiles.Num() != Variant.Value._Params.Get_TileCount() ||
                Variant.Value._Params._Divisions != Default._Params._Divisions)
            {
                Result._Status = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
                Result._ProfileTag = Variant.Key;
                return Result;
            }
        }

        auto SeenTileIndices = TSet<int32>{};
        auto DefaultTransitions = TArray<FDecodedTransition>{};
        auto VariantTransitions = TMap<FGameplayTag, TArray<FDecodedTransition>>{};

        DefaultTransitions.Reserve(InTransitions.Num());

        for (const auto& Variant : InBaseBundle._VariantFields)
        {
            VariantTransitions.Add(Variant.Key).Reserve(InTransitions.Num());
        }

        for (const auto& Transition : InTransitions)
        {
            const auto TileIndex = Get_TileIndex(Default._Params._Divisions, Transition._TileId._Coord);
            const auto IsReplace = Transition._Kind == ECk_GroundNav_StreamTileTransitionKind::Replace;
            const auto IsRemove = Transition._Kind == ECk_GroundNav_StreamTileTransitionKind::Remove;

            if (NOT Transition._TileId.Get_IsValid() || Transition._TileId._VolumeId != InVolumeId)
            {
                Result._Status = ECk_GroundNav_StreamCompositionStatus::InvalidTileId;
                return Result;
            }

            if (NOT Default._Tiles.IsValidIndex(TileIndex))
            {
                Result._Status = ECk_GroundNav_StreamCompositionStatus::TileCoordOutsideLattice;
                return Result;
            }

            if (SeenTileIndices.Contains(TileIndex))
            {
                Result._Status = ECk_GroundNav_StreamCompositionStatus::DuplicateTileCoord;
                return Result;
            }

            SeenTileIndices.Add(TileIndex);

            if ((NOT IsReplace && NOT IsRemove) ||
                (IsReplace && (Transition._DefaultBlob.IsEmpty() ||
                    Transition._VariantBlobs.Num() != InBaseBundle._VariantFields.Num())) ||
                (IsRemove && (NOT Transition._DefaultBlob.IsEmpty() || NOT Transition._VariantBlobs.IsEmpty())))
            {
                Result._Status = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
                return Result;
            }

            auto DefaultTransition = FDecodedTransition{};
            DefaultTransition._Coord = Transition._TileId._Coord;
            DefaultTransition._Kind = Transition._Kind;

            if (IsReplace)
            {
                const auto Status = Decode_ReplaceTile(
                    Default, Transition._DefaultBlob, DefaultTransition._Coord, DefaultTransition._Tile);

                if (Status != ECk_GroundNav_LoadStatus::Loaded)
                {
                    Result._Status = ECk_GroundNav_StreamCompositionStatus::BlobRefused;
                    Result._LoadStatus = Status;
                    return Result;
                }
            }

            DefaultTransitions.Emplace(MoveTemp(DefaultTransition));

            for (const auto& Variant : InBaseBundle._VariantFields)
            {
                const auto* Blob = Transition._VariantBlobs.Find(Variant.Key);

                if (IsReplace && (Blob == nullptr || Blob->IsEmpty()))
                {
                    Result._Status = ECk_GroundNav_StreamCompositionStatus::InvalidTransition;
                    Result._ProfileTag = Variant.Key;
                    return Result;
                }

                auto DecodedTransition = FDecodedTransition{};
                DecodedTransition._Coord = Transition._TileId._Coord;
                DecodedTransition._Kind = Transition._Kind;

                if (IsReplace)
                {
                    const auto Status = Decode_ReplaceTile(
                        Variant.Value, *Blob, DecodedTransition._Coord, DecodedTransition._Tile);

                    if (Status != ECk_GroundNav_LoadStatus::Loaded)
                    {
                        Result._Status = ECk_GroundNav_StreamCompositionStatus::BlobRefused;
                        Result._LoadStatus = Status;
                        Result._ProfileTag = Variant.Key;
                        return Result;
                    }
                }

                VariantTransitions.FindChecked(Variant.Key).Emplace(MoveTemp(DecodedTransition));
            }
        }

        auto Candidate = FCk_GroundNav_StreamFieldBundle{};
        Compose_Profile(Default, DefaultTransitions, Candidate._DefaultField);

        for (const auto& Variant : InBaseBundle._VariantFields)
        {
            auto VariantCandidate = FCk_GroundNav_Field{};
            Compose_Profile(
                Variant.Value,
                VariantTransitions.FindChecked(Variant.Key),
                VariantCandidate);

            Candidate._VariantFields.Emplace(Variant.Key, MoveTemp(VariantCandidate));
        }

        OutBundle = MoveTemp(Candidate);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
