#pragma once

#include "CkGroundNav/Field/CkGroundNav_Field.h"
#include "CkGroundNav/Field/CkGroundNav_FieldSerializeTraits.h"
#include "CkGroundNav/Field/CkGroundNav_FieldTypes.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// A published field as BYTES, and the bytes back as a field.
//
// Hand-rolled over an FArchive rather than reflected. The field types are plain C++ with no
// GENERATED_BODY, and reflecting them would drag a .generated.h into the bake layer, which is the one
// thing that layer is kept free of. Every scalar is written out by name and by size, so the format is
// this file rather than whatever the engine's struct serializer does this year.
//
// The layout is LITTLE-ENDIAN FIXED-WIDTH - every scalar is the writing machine's own bytes at its own
// size - so a blob written here is not readable by a big-endian reader, and this format makes no claim
// to be.
//
// WHAT IS PERSISTED IS THE BAKE PRODUCT AND THE PARAMS THAT PRODUCED IT: the tiles as the field holds
// them, and _Params. Everything a composition DERIVES - the seam portals, the tile edge boundary, the
// resolved links, the plate offsets, the reachability labels and the per-component open flags - is
// re-derived by the reader through the same pure derives a bake runs, so a loaded field cannot carry a
// crossing or a label the tiles beside it do not support. That also keeps the format small: the
// derived arrays are the large half of a field and none of them is an independent fact.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    // 'C','K','G','N'. The first four bytes of every blob, so a file that is not one is refused before
    // anything else is read.
    inline constexpr uint32 kFieldBlobMagic = 0x4E474B43;

    /**
     * The on-disk shape of a blob. BUMPED WHENEVER A BYTE MOVES - a field written by an older writer
     * is refused with a status rather than decoded by a reader that would read the wrong members.
     */
    inline constexpr int32 kFieldBlobFormatVersion = 1;

    /**
     * Where the header's UTC cook seconds begin.
     *
     * Published because it is the ONE run of bytes two writes of the same field are allowed to differ
     * in, and anything comparing two blobs has to know which bytes to skip.
     */
    inline constexpr int32 kFieldBlobCookSecondsOffset = 9;

    /**
     * What a blob's body holds.
     *
     * Written into the header so a whole-field blob and a single-tile blob can never be mistaken for
     * one another by a reader expecting the other; the mistake answers WrongMagic, which is what it is.
     */
    enum class ECk_GroundNav_BlobContent : uint8
    {
        WholeField,
        SingleTile
    };

    // ----------------------------------------------------------------------------------------------------------------
    //
    // The fence itself is in Field/CkGroundNav_FieldSerializeTraits.h - it judges no field type, so
    // something that persists a value which is not one of these can reach it without reaching this
    // file. What follows is this format's own side of it: one assertion per persisted type, naming the
    // members the writer writes, and one specialisation per type so a composite made of them passes in
    // turn. A member added to any of these types and not to its list here is a member the blob loses.
    //
    // A member whose TYPE is a tag is named by the type the blob carries in its place - an int32 index
    // into the name table, or an array of them - because that is what is actually persisted.

    static_assert(kPersistableMembersAreValues<decltype(FCk_GroundNav_BodyRef::_Value)>,
        "FCk_GroundNav_BodyRef must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_BodyRef>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<decltype(FCk_GroundNav_Epoch::_Value)>,
        "FCk_GroundNav_Epoch must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_Epoch>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_TileCoord::_X), decltype(FCk_GroundNav_TileCoord::_Y)>,
        "FCk_GroundNav_TileCoord must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_TileCoord>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_TileBakeStats::_SourceTriangleCount),
            decltype(FCk_GroundNav_TileBakeStats::_RasterizedSpanCount),
            decltype(FCk_GroundNav_TileBakeStats::_RejectedCellCount)>,
        "FCk_GroundNav_TileBakeStats must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_TileBakeStats>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_Plate::_LayerIndex), decltype(FCk_GroundNav_Plate::_MinX),
            decltype(FCk_GroundNav_Plate::_MinY), decltype(FCk_GroundNav_Plate::_MaxX),
            decltype(FCk_GroundNav_Plate::_MaxY), decltype(FCk_GroundNav_Plate::_MaxPlaneResidualUu),
            decltype(FCk_GroundNav_Plate::_HeightRangeUu), decltype(FCk_GroundNav_Plate::_MinClearanceUu),
            decltype(FCk_GroundNav_Plate::_AreaPolicyIndex), decltype(FCk_GroundNav_Plate::_CostMultiplier)>,
        "FCk_GroundNav_Plate must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_Plate>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_ClearanceField::_SizeX), decltype(FCk_GroundNav_ClearanceField::_SizeY),
            decltype(FCk_GroundNav_ClearanceField::_LayerCount),
            decltype(FCk_GroundNav_ClearanceField::_CellSizeUu),
            decltype(FCk_GroundNav_ClearanceField::_Cells)>,
        "FCk_GroundNav_ClearanceField must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_ClearanceField>
    {
        static constexpr bool Value = true;
    };

    // _AreaPolicies is the reason the name table exists at all: a container of tags is not a value a
    // blob may carry, so each container travels as the run of table indices named last here.
    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_PlateField::_SizeX), decltype(FCk_GroundNav_PlateField::_SizeY),
            decltype(FCk_GroundNav_PlateField::_LayerCount), decltype(FCk_GroundNav_PlateField::_Plates),
            decltype(FCk_GroundNav_PlateField::_CellToPlate), TArray<TArray<int32>>>,
        "FCk_GroundNav_PlateField must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_PlateField>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_Portal::_PlateA), decltype(FCk_GroundNav_Portal::_PlateB),
            decltype(FCk_GroundNav_Portal::_Direction), decltype(FCk_GroundNav_Portal::_FromMin),
            decltype(FCk_GroundNav_Portal::_FromMax), decltype(FCk_GroundNav_Portal::_MinEndZUu),
            decltype(FCk_GroundNav_Portal::_MaxEndZUu), decltype(FCk_GroundNav_Portal::_TraversalClearanceUu)>,
        "FCk_GroundNav_Portal must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_Portal>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_PortalField::_Portals), TArray<TArray<int32>>>,
        "FCk_GroundNav_PortalField must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_PortalField>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_BoundarySegment::_PlateIndex),
            decltype(FCk_GroundNav_BoundarySegment::_LayerIndex),
            decltype(FCk_GroundNav_BoundarySegment::_Side), decltype(FCk_GroundNav_BoundarySegment::_FromCell),
            decltype(FCk_GroundNav_BoundarySegment::_ToCell), decltype(FCk_GroundNav_BoundarySegment::_Start),
            decltype(FCk_GroundNav_BoundarySegment::_End),
            decltype(FCk_GroundNav_BoundarySegment::_InwardNormalXY)>,
        "FCk_GroundNav_BoundarySegment must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_BoundarySegment>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_BoundaryField::_Segments),
            decltype(FCk_GroundNav_BoundaryField::_EdgeCandidates),
            decltype(FCk_GroundNav_BoundaryField::_BucketsX),
            decltype(FCk_GroundNav_BoundaryField::_BucketsY), TArray<TArray<int32>>>,
        "FCk_GroundNav_BoundaryField must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_BoundaryField>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_SeamStub::_Direction), decltype(FCk_GroundNav_SeamStub::_AlongIndex),
            decltype(FCk_GroundNav_SeamStub::_PlateIndex), decltype(FCk_GroundNav_SeamStub::_NearSurfaceZUu),
            decltype(FCk_GroundNav_SeamStub::_FarSurfaceZUu), decltype(FCk_GroundNav_SeamStub::_ClearanceUu)>,
        "FCk_GroundNav_SeamStub must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_SeamStub>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_Tile::_Coord), decltype(FCk_GroundNav_Tile::_Epoch),
            decltype(FCk_GroundNav_Tile::_Status), decltype(FCk_GroundNav_Tile::_Origin),
            decltype(FCk_GroundNav_Tile::_CellSizeUu), decltype(FCk_GroundNav_Tile::_MaxClearanceUu),
            decltype(FCk_GroundNav_Tile::_SizeX), decltype(FCk_GroundNav_Tile::_SizeY),
            decltype(FCk_GroundNav_Tile::_LayerCount), decltype(FCk_GroundNav_Tile::_SurfaceZ),
            decltype(FCk_GroundNav_Tile::_Clearance), decltype(FCk_GroundNav_Tile::_Plates),
            decltype(FCk_GroundNav_Tile::_Portals), decltype(FCk_GroundNav_Tile::_Boundary),
            decltype(FCk_GroundNav_Tile::_SeamStubs), decltype(FCk_GroundNav_Tile::_BakeStats)>,
        "FCk_GroundNav_Tile must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_Tile>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_OpenBody::_Body), decltype(FCk_GroundNav_OpenBody::_Description),
            decltype(FCk_GroundNav_OpenBody::_Bounds), decltype(FCk_GroundNav_OpenBody::_TriangleCount),
            decltype(FCk_GroundNav_OpenBody::_OpenEdgeCount),
            decltype(FCk_GroundNav_OpenBody::_OpenEdgePoints)>,
        "FCk_GroundNav_OpenBody must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_OpenBody>
    {
        static constexpr bool Value = true;
    };

    // The authored half. These are reflected types whose members are private, so each one is named
    // through the getter that is the only way to reach it.

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_ShapeBox_Dimensions{}.Get_HalfExtents()),
            decltype(FCk_ShapeBox_Dimensions{}.Get_ConvexRadius())>,
        "FCk_ShapeBox_Dimensions must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_ShapeBox_Dimensions>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_ShapeCapsule_Dimensions{}.Get_HalfHeight()),
            decltype(FCk_ShapeCapsule_Dimensions{}.Get_Radius())>,
        "FCk_ShapeCapsule_Dimensions must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_ShapeCapsule_Dimensions>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_ShapeCylinder_Dimensions{}.Get_HalfHeight()),
            decltype(FCk_ShapeCylinder_Dimensions{}.Get_Radius()),
            decltype(FCk_ShapeCylinder_Dimensions{}.Get_ConvexRadius())>,
        "FCk_ShapeCylinder_Dimensions must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_ShapeCylinder_Dimensions>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<decltype(FCk_ShapeSphere_Dimensions{}.Get_Radius())>,
        "FCk_ShapeSphere_Dimensions must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_ShapeSphere_Dimensions>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_AnyShape{}.Get_ShapeType()), decltype(FCk_AnyShape{}.Get_Box()),
            decltype(FCk_AnyShape{}.Get_Capsule()), decltype(FCk_AnyShape{}.Get_Cylinder()),
            decltype(FCk_AnyShape{}.Get_Sphere())>,
        "FCk_AnyShape must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_AnyShape>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_BakeConfig{}.Get_CellSizeUu()),
            decltype(FCk_GroundNav_BakeConfig{}.Get_CellHeightUu()),
            decltype(FCk_GroundNav_BakeConfig{}.Get_TileSizeUu()),
            decltype(FCk_GroundNav_BakeConfig{}.Get_MaxColumnsPerTile())>,
        "FCk_GroundNav_BakeConfig must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_BakeConfig>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_AgentProfile{}.Get_StandingExtents()),
            decltype(FCk_GroundNav_AgentProfile{}.Get_MaxSlopeDegrees()),
            decltype(FCk_GroundNav_AgentProfile{}.Get_MaxSlopeChangeDegrees()),
            decltype(FCk_GroundNav_AgentProfile{}.Get_StepHeightUu()),
            decltype(FCk_GroundNav_AgentProfile{}.Get_LedgeSensitivity()),
            decltype(FCk_GroundNav_AgentProfile{}.Get_RoughPerchToleranceUu())>,
        "FCk_GroundNav_AgentProfile must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_AgentProfile>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_MergeTunables{}.Get_PlaneFitToleranceUu()),
            decltype(FCk_GroundNav_MergeTunables{}.Get_NormalConeDegrees())>,
        "FCk_GroundNav_MergeTunables must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_MergeTunables>
    {
        static constexpr bool Value = true;
    };

    // _AreaTag travels as the int32 named last: an authored tag is a name, and a name is what the
    // table carries.
    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_MarkupRecord{}.Get_Id()), decltype(FCk_GroundNav_MarkupRecord{}.Get_Shape()),
            decltype(FCk_GroundNav_MarkupRecord{}.Get_WorldTransform()),
            decltype(FCk_GroundNav_MarkupRecord{}.Get_Kind()), decltype(FCk_GroundNav_MarkupRecord{}.Get_Enable()),
            decltype(FCk_GroundNav_MarkupRecord{}.Get_CostMultiplier()),
            decltype(FCk_GroundNav_MarkupRecord{}.Get_RequestedAtEpoch()), int32>,
        "FCk_GroundNav_MarkupRecord must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_MarkupRecord>
    {
        static constexpr bool Value = true;
    };

    // _AreaTag and _UserTypeTag travel as the two int32s named last, for the reason the markup
    // record's does.
    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_LinkRecord{}.Get_Id()), decltype(FCk_GroundNav_LinkRecord{}.Get_Start()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_End()), decltype(FCk_GroundNav_LinkRecord{}.Get_Direction()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_CostMultiplierForward()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_CostMultiplierBackward()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_ClearanceUu()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_Enable()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_ProjectionMode()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_ProjectionHorizontalExtentUu()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_ProjectionVerticalExtentUu()),
            decltype(FCk_GroundNav_LinkRecord{}.Get_RequestedAtEpoch()), int32, int32>,
        "FCk_GroundNav_LinkRecord must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_LinkRecord>
    {
        static constexpr bool Value = true;
    };

    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_FieldParams::_OriginXY), decltype(FCk_GroundNav_FieldParams::_Divisions),
            decltype(FCk_GroundNav_FieldParams::_MinZUu), decltype(FCk_GroundNav_FieldParams::_MaxZUu),
            decltype(FCk_GroundNav_FieldParams::_Config), decltype(FCk_GroundNav_FieldParams::_Profile),
            decltype(FCk_GroundNav_FieldParams::_MergeTunables),
            decltype(FCk_GroundNav_FieldParams::_MarkupRecords), decltype(FCk_GroundNav_FieldParams::_Links),
            decltype(FCk_GroundNav_FieldParams::_MaxClearanceUu)>,
        "FCk_GroundNav_FieldParams must stay value-only - a blob outlives the process that wrote it");

    template <>
    struct TIsPersistableValue<FCk_GroundNav_FieldParams>
    {
        static constexpr bool Value = true;
    };

    // The derived arrays are deliberately absent: they are not persisted, and a member added to this
    // list that the writer does not write is a member a load would invent.
    static_assert(kPersistableMembersAreValues<
            decltype(FCk_GroundNav_Field::_Params), decltype(FCk_GroundNav_Field::_Tiles),
            decltype(FCk_GroundNav_Field::_Epoch), decltype(FCk_GroundNav_Field::_OpenBodies)>,
        "FCk_GroundNav_Field must stay value-only - a blob outlives the process that wrote it");

    // ----------------------------------------------------------------------------------------------------------------

    /**
     * The whole field as bytes: its params, every tile, and the open-body diagnostics the bake found.
     *
     * Deterministic for a given field. Two writes in one process produce identical bytes but for the
     * header's cook date, and two fields whose content matches produce identical name tables however
     * their tags were interned, because the table is sorted by string rather than by first sighting.
     */
    CKGROUNDNAV_API auto
    Write_Field(
        const FCk_GroundNav_Field& InField,
        TArray<uint8>&             OutBlob) -> void;

    /**
     * One tile as bytes, against the lattice the header names.
     *
     * The blob carries no params of its own: a tile is read INTO a field that already has them, and
     * the header's lattice is the part of the params that has to agree for the read to mean anything.
     * A coord the field does not have writes an Unbuilt tile at that coord, which is what the field
     * itself would say about it.
     */
    CKGROUNDNAV_API auto
    Write_Tile(
        const FCk_GroundNav_Field&     InField,
        const FCk_GroundNav_TileCoord& InCoord,
        TArray<uint8>&                 OutBlob) -> void;

    /**
     * A whole-field blob covering only the tiles named, on the SAME lattice.
     *
     * Every other tile is written Unbuilt - present, carrying its coord, and holding no cells and no
     * plates - so the blob loads as a valid field of the same lattice with fewer built tiles rather
     * than as a smaller field whose indices mean something else. Nothing is renumbered.
     *
     * The crossings and links that touched an absent tile are gone from the loaded field for free: the
     * reader re-derives both, an unbuilt neighbour yields no seam portal, and a link end over unbuilt
     * ground resolves to nothing and is counted.
     */
    CKGROUNDNAV_API auto
    Write_FieldSubset(
        const FCk_GroundNav_Field&             InField,
        const TArray<FCk_GroundNav_TileCoord>& InKeptTiles,
        TArray<uint8>&                         OutBlob) -> void;

    /**
     * A whole-field blob back into a field, with every derived array re-derived after the flat arrays
     * are read.
     *
     * OutField IS NOT TOUCHED UNLESS THE READ SUCCEEDS. A refused blob leaves the caller holding
     * exactly the field it had, which is the only behaviour a caller can write a fallback against.
     */
    CKGROUNDNAV_API auto
    Read_Field(
        const TArray<uint8>& InBlob,
        FCk_GroundNav_Field& OutField) -> ECk_GroundNav_LoadStatus;

    /**
     * Whether a tile read composes the field it landed in before it returns.
     *
     * The derives are WHOLE-FIELD - the seam portals, the links and the reachability labels are all
     * re-derived over every tile - so a caller reading N tiles into one field with the default runs
     * them N times and keeps only the last run's answer. Deferred lets such a caller pay for one, and
     * it OWES Compose_LoadedField afterwards: a field left uncomposed carries tiles no crossing, no
     * resolved link and no label supports, which is a field that answers nothing.
     */
    enum class ECk_GroundNav_ComposeOnLoad : uint8
    {
        Now,
        Deferred
    };

    /**
     * The derives a load owes, over a field whose tiles are all in place.
     *
     * Public so a caller that read its tiles Deferred can settle the debt, and the same call the
     * readers make so a composed-by-hand field cannot differ from a composed-by-read one.
     */
    CKGROUNDNAV_API auto
    Compose_LoadedField(
        FCk_GroundNav_Field& InOutField) -> void;

    /**
     * One tile blob into an existing field, replacing whatever that field held at the tile's coord and
     * re-deriving the whole field afterwards.
     *
     * THE LATTICE MUST MATCH. A tile carries nothing but tile-local indices, so one read into a field
     * divided differently would be cells placed against a lattice that never produced them; that is
     * LatticeMismatch, and the field is left alone.
     *
     * A REFUSED read composes nothing whatever InCompose says: the field was not changed, so there is
     * nothing to re-derive.
     */
    CKGROUNDNAV_API auto
    Read_TileInto(
        const TArray<uint8>&        InBlob,
        FCk_GroundNav_Field&        InOutField,
        ECk_GroundNav_ComposeOnLoad InCompose = ECk_GroundNav_ComposeOnLoad::Now) -> ECk_GroundNav_LoadStatus;

    /**
     * The blob's name table alone, in the order it is written - ascending by string.
     *
     * Reads the header and stops. What it answers is which tags a blob names, without decoding a field
     * to find out, and it is the one place the sortedness of the table is observable from outside.
     *
     * OutTagNames SURVIVES an UnknownTag refusal, and that one alone: the whole table was read, and
     * which of the names it holds failed to resolve is exactly what a caller asks this for. WrongMagic,
     * WrongVersion, Truncated and Corrupt all mean the table itself is not trustworthy, and clear it.
     */
    CKGROUNDNAV_API auto
    Read_TagTable(
        const TArray<uint8>& InBlob,
        TArray<FString>&     OutTagNames) -> ECk_GroundNav_LoadStatus;
}

// --------------------------------------------------------------------------------------------------------------------
