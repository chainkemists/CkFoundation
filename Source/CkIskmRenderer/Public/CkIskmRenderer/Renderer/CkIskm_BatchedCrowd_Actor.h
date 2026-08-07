#pragma once

#include "GameFramework/Actor.h"

#include "CkIskm_BatchedClusterComponent.h" // for UCk_Iskm_BatchedClusterComponent::FInstance

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h" // FCk_Handle_Transform / FCk_Handle (member cosmetics + controller entity)

#include "CkUsf/Stylize/CkUsf_CelShade_Params.h" // ECk_Usf_CelPattern (member-indexed cel patterns)

#include "CkIskm_BatchedCrowd_Actor.generated.h"

class UCk_IskmAnimCollection_Data;
class USceneComponent;
class UCkUsf_OutlinePreset;

// --------------------------------------------------------------------------------------------------------------------
// CkIskmRenderer Plan-2 — spatial tile manager for a batched GPU-skinned crowd. Architecture, tuning and the
// distance-LOD flip contract: CkIskmRenderer/CLAUDE.md § Plan-2 production guide.
//
// SINGLE SOURCE OF TRUTH: this manager owns all member state (transform, animation time/frames, visibility,
// custom data) and ticks it itself; tile components are render-facing views with self-tick disabled.
//
// Usage: Initialize() -> AddInstance() x N -> Finalize(), then drive members via Set_Member*.
UCLASS(NotBlueprintable)
class CKISKMRENDERER_API ACk_Iskm_BatchedCrowd_Actor : public AActor
{
    GENERATED_BODY()

public:
    friend class UCk_IskmRenderer_Subsystem_UE;

public:
    ACk_Iskm_BatchedCrowd_Actor();

    void Initialize(UCk_IskmAnimCollection_Data* InCollection, float InTileSize);

    // Buffer one instance at a world transform; assigned to its tile (created on demand). Flushed by Finalize().
    void AddInstance(const FTransform& InWorldTransform, int32 InSequenceIndex, float InRate, float InTimeOffset);

    // Flush all members into their tile clusters. Call once after all AddInstance().
    void Finalize();

    int32 Get_TileCount() const { return _Tiles.Num(); }
    int32 Get_InstanceCount() const { return _Members.Num(); }

    // ---- Member API (game-facing: NPC/crowd systems drive these) ----
    int32      Get_MemberCount() const { return _Members.Num(); }
    FTransform Get_MemberWorldTransform(int32 InIndex) const;
    int32      Get_MemberSequenceIndex(int32 InIndex) const;
    bool       Get_MemberVisible(int32 InIndex) const;

    // In-tile moves ride the light per-frame push; crossing a tile border migrates the member (rebuilds both).
    void       Set_MemberTransform(int32 InIndex, const FTransform& InWorldTransform);

    // Switch a member's sequence/rate (e.g. idle -> walk when its NPC starts moving).
    void       Set_MemberAnimation(int32 InIndex, int32 InSequenceIndex, float InRate, bool InResetTime);

    // Per-member material custom data, legacy 2-float form — writes UserData[0]/[1] (shader floats [2]/[3]).
    void       Set_MemberCustomData(int32 InIndex, float InA, float InB);

    // InFirstFloat is the material PerInstanceCustomData DataIndex (game data: 2..15; [0]/[1] are the
    // animation frame bits and are rejected — writing them would corrupt pose lookup).
    void       Set_MemberCustomData(int32 InIndex, int32 InFirstFloat, const TArray<float>& InValues);
    float      Get_MemberCustomData(int32 InIndex, int32 InFloatIndex) const;

    // World-space transform of a baked socket at the member's CURRENT animation frame (far cosmetics — the
    // crowd owns the clock). False when the collection bakes no such socket or the index is invalid.
    bool       TryGet_MemberSocketTransform(int32 InIndex, FName InSocket, FTransform& OutWorld) const;

    // Hide/show a member in its batched tile (rebuilds that one tile), leaving a gap for a per-SKMC stand-in.
    void       Set_MemberVisible(int32 InIndex, bool InVisible);

    // Default CustomPrimitiveData floats applied to EVERY tile component, existing and future. One shared
    // value set per crowd; per-member variation needs the per-instance floats via Set_MemberCustomData.
    void       Set_DefaultTileCustomPrimitiveData(const TArray<float>& InFloats);

    // One material applied to EVERY slot of EVERY tile, existing and future. Null restores mesh materials.
    void       Set_OverrideMaterial(UMaterialInterface* InMaterial);

    // Per-SLOT override materials: element index = mesh material slot. Set_OverrideMaterial above WINS over
    // these; null/absent entries fall back to mesh defaults. Applied to existing and future tiles.
    void       Set_SlotOverrideMaterials(const TArray<UMaterialInterface*>& InMaterials);
    UMaterialInterface* Get_SlotOverrideMaterial(int32 InSlotIndex) const;

    // Sum of each tile component's CURRENT instance count (i.e. only visible members) — drops when a member is hidden.
    int32      Get_RenderedInstanceCount() const;

    // ---- Entity outline (member-indexed — batched members are not entities; see CkUsf/Claude.md § Entity outlines) ----
    // One custom-depth-only "highlight cluster" per (crowd, preset) at the world origin, pushed every
    // AdvanceAnimation. Hidden members (Plan-1 flip stand-ins) are excluded — outline those via the entity API.

    // Outline a member with InPreset (replaces any previous preset). Null preset = Clear_MemberOutline.
    // The outline WINS over both lower claims on the same member: an existing cel pattern or stylize mask
    // is cleared first.
    void       Set_MemberOutline(int32 InIndex, UCkUsf_OutlinePreset* InPreset);
    void       Clear_MemberOutline(int32 InIndex);
    UCkUsf_OutlinePreset* Get_MemberOutlinePreset(int32 InIndex) const;
    int32      Get_OutlinedMemberCount() const;
    // Instances currently in the highlight clusters (visible outlined members only) — test/gym surface.
    int32      Get_OutlineRenderedInstanceCount() const;

    // ---- Entity cel pattern (member-indexed) — the outline's twin, see CkUsf/Claude.md § Cel shade ----
    // Same highlight-cluster machinery, one cluster per (crowd, stencil value): the cel contract is a
    // DIRECT stencil value rather than a refcounted preset allocation, so nothing is allocated here and
    // nothing is released. REJECTED on an outlined member — both write the same Custom-Stencil byte — and
    // it WINS over a stylize mask on the same member, clearing it first.

    void       Set_MemberCelPattern(int32 InIndex, ECk_Usf_CelPattern InPattern);
    void       Clear_MemberCelPattern(int32 InIndex);
    // A cel pattern has no "invalid" value to report absence with, so the caller states what absence means.
    ECk_Usf_CelPattern Get_MemberCelPatternOr(int32 InIndex, ECk_Usf_CelPattern InFallback) const;
    // Custom-Stencil value the member's cluster writes, 0 when it carries no pattern — test/gym surface.
    int32      Get_MemberCelPatternStencilValue(int32 InIndex) const;
    int32      Get_CelPatternedMemberCount() const;
    int32      Get_CelPatternRenderedInstanceCount() const;

    // ---- Entity stylize effect mask (member-indexed) — see CkUsf/Claude.md § Stylize ----
    // The cel pattern's twin on the same highlight-cluster machinery, one cluster per (crowd, mask stencil
    // value) — and since the mask value is project-wide, that is normally one cluster per crowd. Nothing is
    // allocated and nothing is released. REFUSED on an outlined OR patterned member: the mask is the LAST of
    // the three claims on the Custom-Stencil byte.

    void       Set_MemberStylizeMask(int32 InIndex);
    void       Clear_MemberStylizeMask(int32 InIndex);
    bool       Get_IsMemberStylizeMasked(int32 InIndex) const;
    int32      Get_StylizeMaskedMemberCount() const;
    int32      Get_StylizeMaskRenderedInstanceCount() const;

    //~ AActor
    virtual void EndPlay(const EEndPlayReason::Type InEndPlayReason) override;
    // Editor-world actors never BeginPlay/EndPlay — controller-entity cleanup happens here for them.
    virtual void Destroyed() override;

#if WITH_EDITOR
public:
    // Editor preview crowds redirect viewport clicks on their tile instances to the placed actor whose
    // preview they render (ck::FFragment_EditorSelectionOwner), so the mesh selects like its billboard.
    auto
    IsSelectionChild() const -> bool override;

    auto
    GetSelectionParent() const -> AActor* override;
#endif

#if WITH_EDITORONLY_DATA
private:
    // Non-UPROPERTY on purpose: this actor is transient and must never round-trip through the
    // transaction buffer; the owner is a level actor whose lifetime the weak ptr observes.
    TWeakObjectPtr<AActor> _EditorSelectionOwner;
#endif

public:

    // ---- ECS-clock advance (the actor no longer self-ticks) ----

    // Advance every member's animation time/frame, push dirty tiles, then push the outline groups. Replaces
    // AActor::Tick; driven once per frame by FProcessor_IskmCrowd_Advance so member render, far-cosmetic
    // placement and the highlight clusters all resolve on the SAME ECS clock.
    void       AdvanceAnimation(float InDeltaTime);

    // Place every registered member cosmetic at its baked socket for the member's CURRENT frame — lockstep
    // with the PushTile that AdvanceAnimation just did.
    void       DriveCosmetics();

    // A cosmetic ISM entity rides InIndex's baked socket while its member is far. Replace-if-same-entity.
    void       Register_MemberCosmetic(int32 InIndex, const FCk_Handle_Transform& InCosmetic, FName InSocket, const FTransform& InRelOffset);

    // Drop all cosmetics registered to InIndex (promote / hide / slot release). Idempotent.
    void       Clear_MemberCosmetics(int32 InIndex);

private:
    struct FMember
    {
        FTransform WorldXf = FTransform::Identity;
        FIntPoint  Tile = FIntPoint(0, 0);
        UCk_Iskm_BatchedClusterComponent::FInstance Inst;
        bool       Visible = true;
    };

    struct FMemberCosmetic
    {
        FCk_Handle_Transform Cosmetic;
        FName                Socket    = NAME_None;
        FTransform           RelOffset = FTransform::Identity;
    };

    // Idempotent — called from both EndPlay (runtime) and Destroyed (editor).
    void      DoDestroy_ControllerEntity();

    FIntPoint TileCoordOf(const FVector& InWorldLocation) const;
    FVector   TileCentre(const FIntPoint& InTile) const;
    UCk_Iskm_BatchedClusterComponent* GetOrCreate_Tile(const FIntPoint& InTile);
    // Full rebuild (Set_Instances — proxy recreate). Use for count changes: visibility flips + tile migration.
    void      RebuildTile(const FIntPoint& InTile);
    // Light push (same instance count — transforms/frames/custom data only). Rolls members' velocity history.
    void      PushTile(const FIntPoint& InTile);
    FBox      TileLocalBounds() const;

    UPROPERTY(Transient)
    TObjectPtr<USceneComponent> _Root;

    UPROPERTY(Transient)
    TObjectPtr<UCk_IskmAnimCollection_Data> _Collection;

    float _TileSize = 2000.0f;

    TArray<float> _DefaultTileCustomPrimitiveData;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> _OverrideMaterial;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> _SlotOverrideMaterials;

    UPROPERTY(Transient)
    TMap<FIntPoint, TObjectPtr<UCk_Iskm_BatchedClusterComponent>> _Tiles;

    // Member indices per tile (includes hidden — filtered when building the render arrays).
    TMap<FIntPoint, TArray<int32>> _TileMembers;

    TSet<FIntPoint> _DirtyTiles;

    TArray<FMember> _Members;

    // ---- custom-depth highlight state (outlines + cel patterns share this machinery) ----

    // Not reflected (nested plain struct): the cluster component is kept alive by the actor's
    // OwnedComponents; presets are referenced by weak ptr (dead preset ⇒ group torn down lazily).
    struct FHighlightGroup
    {
        TWeakObjectPtr<UCk_Iskm_BatchedClusterComponent> Comp;
        TArray<int32> Members;                    // member indices (hidden ones filtered at build/push)
        uint8 Stencil = 0;                        // the Custom-Stencil this cluster writes
        FBox PaddedBounds = FBox(ForceInit);      // world box the fixed bounds cover; exceed → rebuild
    };

    // The ONE construction path for both highlight kinds: a custom-depth-only cluster at the world
    // origin carrying InStencilValue. Null (and an ensure) when the crowd has no collection/mesh.
    UCk_Iskm_BatchedClusterComponent* DoCreate_HighlightCluster(uint8 InStencilValue, const TCHAR* InNameBase);
    // Full rebuild of a group (Set_Instances + recomputed fixed bounds — proxy recreate).
    void Rebuild_HighlightGroup(FHighlightGroup& InGroup);
    // Light per-frame push of live member data into the group's cluster (no bounds work).
    void Push_HighlightGroup(FHighlightGroup& InGroup);
    // The group InIndex belongs to, or null. A member carries at most one — outline and cel pattern
    // write the same Custom-Stencil byte and are mutually exclusive.
    FHighlightGroup* DoFind_MemberHighlightGroup(int32 InIndex);

    TMap<TWeakObjectPtr<UCkUsf_OutlinePreset>, FHighlightGroup> _OutlineGroups;
    TMap<int32, TWeakObjectPtr<UCkUsf_OutlinePreset>> _MemberOutlines;

    // Cel groups are keyed on the resolved stencil VALUE (the ISM shadow's keying — the cel contract is a
    // direct value, so two patterns are two clusters and nothing is refcounted). The member records the
    // key it joined alongside the pattern it asked for, so a StencilBase change between Set and Clear
    // still resolves the group the member is actually in.
    struct FMemberCelPattern
    {
        ECk_Usf_CelPattern Pattern = ECk_Usf_CelPattern::Bayer;
        uint8              Stencil = 0;
    };

    TMap<uint8, FHighlightGroup> _CelGroups;
    TMap<int32, FMemberCelPattern> _MemberCelPatterns;

    // Mask groups key on the resolved stencil VALUE like the cel groups, but the mask has no per-member
    // payload — membership IS the state — so a set replaces the cel path's member map. A member therefore
    // records no key of its own, and its group is found by membership: that is what keeps a project-setting
    // change between Set and Clear from stranding the cluster the member is actually in.
    TMap<uint8, FHighlightGroup> _StylizeMaskGroups;
    TSet<int32> _StylizeMaskedMembers;

    // Non-UPROPERTY: ECS handles are value handles into the registry, not GC-tracked (matches _Members).
    TMap<int32, TArray<FMemberCosmetic>> _MemberCosmetics;

    // FProcessor_IskmCrowd_Advance iterates this to drive AdvanceAnimation + DriveCosmetics.
    FCk_Handle _ControllerEntity;
};
