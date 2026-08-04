#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "CkParticles/DataInterface/CkParticles_PartTuning.h"

#include "CkParticles_TuningDefinition.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// A reusable tuning preset, in two layers.
//
// The GLOBAL floats pack into the User.CkTuning float4 the DI's ExecuteStage applies centrally (see
// CkParticles_Behaviors.ush), so any behavior on any template is tunable through this asset without a Behavior_*.ush
// knowing it exists. All-ones is the identity, which renders exactly the untuned behavior.
//
// The PART rows tune the layers INSIDE one effect, one row per renderer the behavior draws through. The roster of
// rows is generator-owned (Generate Tuning Assets fills and reconciles it from the behavior's renderer band) and the
// values in them are the designer's — a regeneration never touches a value.
// --------------------------------------------------------------------------------------------------------------------

// One tunable layer of one behavior. The row's IDENTITY is its VisTag, which is what the reconcile pass matches on;
// _PartName is a generator-refreshed label. Every value here is an identity by default, so a row nobody edited
// renders exactly the untuned layer.
USTRUCT(BlueprintType)
struct CKPARTICLES_API FCkParticles_PartTuning_AssetRow
{
    GENERATED_BODY()

public:
    // Which layer this row tunes, in the behavior's own words. Written by the generator — editing it changes nothing.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CkParticles")
    FName _PartName;

    // The renderer tag the layer draws through. The row's real identity; also generator-written.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CkParticles")
    int32 _VisTag = 0;

    // Multiplies the layer's sprite size and its mesh scale.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _SizeMultiplier = 1.0f;

    // Stretches a velocity-aligned streak along its motion, on top of the size multiplier.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _StretchMultiplier = 1.0f;

    // Multiplies a mesh layer's carrier scale only; a sprite layer ignores it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _MeshScaleMultiplier = 1.0f;

    // Multiplies how fast the layer's particles travel.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _SpeedMultiplier = 1.0f;

    // Recolours the layer — the RGB multiplies the layer's colour. The ALPHA IS IGNORED: fade the layer with
    // _AlphaMultiplier instead, so a tint picked in the colour wheel never dims it by accident.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    FLinearColor _Tint = FLinearColor::White;

    // Fades the layer in or out without touching its colour.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _AlphaMultiplier = 1.0f;

    // Scales how hard the layer's material erodes away; above 1 it dissolves sooner.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _DissolveMultiplier = 1.0f;

    // Scales the layer's material distortion — the heat-haze/refraction wobble.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _DistortionMultiplier = 1.0f;

    // Scales how fast the layer's texture pans across itself.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _UvPanMultiplier = 1.0f;

    // Scales the layer's emissive boost — how much it blooms.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _EmissiveMultiplier = 1.0f;

    // Spins the layer's sprites by a fixed angle, in degrees.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _RotationOffsetDegrees = 0.0f;

    // Fraction of the part's life it is visible; raise _WindowStart to delay the layer's entrance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _WindowStart = 0.0f;

    // Fraction of the part's life it is visible; trim the tail by lowering _WindowEnd.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _WindowEnd = 1.0f;

    // Nudges the whole layer in the effect's own space, in centimetres.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    FVector _PositionOffset = FVector::ZeroVector;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKPARTICLES_API UCkParticles_TuningDefinition : public UDataAsset
{
    GENERATED_BODY()

public:
    // Multiplies the behavior's Size (sprite) and Scale (mesh carrier).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _SizeMultiplier = 1.0f;

    // Multiplies the behavior's Color.rgb — brightness, and with additive materials, apparent intensity.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _ColorIntensity = 1.0f;

    // Multiplies the behavior's Color.a.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _AlphaMultiplier = 1.0f;

    // Multiplies the Age and DeltaTime the behavior reads, so its arc plays faster or slower. Niagara still retires
    // particles at their REAL lifetime, which this does not touch: above 1 the arc finishes early and holds its end
    // state until the particle dies, below 1 the particle dies before the arc completes and the tail is cut.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    float _PlaybackSpeed = 1.0f;

    // Recolours the WHOLE effect. There are no tint slots in the global User.CkTuning float4, so this is folded into
    // the part rows at conversion time — every part's tint is multiplied by it — which also means it only reaches a
    // behavior through _Parts. Alpha is ignored here for the same reason it is on a row.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CkParticles")
    FLinearColor _GlobalTint = FLinearColor::White;

    // One row per layer the behavior draws. Fixed size on purpose: the generator owns the roster (it is derived from
    // the behavior's renderer band), so a designer edits values but cannot add or remove a layer that does not exist.
    UPROPERTY(EditAnywhere, EditFixedSize, BlueprintReadWrite, Category = "CkParticles",
        meta = (TitleProperty = "_PartName"))
    TArray<FCkParticles_PartTuning_AssetRow> _Parts;

public:
    // The rows packed into the block the DI reads, addressed against the behavior's own VisTag band. A row whose
    // VisTag the behavior no longer declares is SKIPPED with a warning rather than aliased onto another layer —
    // that is a stale asset the generator has not reconciled yet.
    auto Get_AsPartTuningBlock(
        int32 InBehaviorId) const -> FCkParticles_PartTuningBlock;
};
