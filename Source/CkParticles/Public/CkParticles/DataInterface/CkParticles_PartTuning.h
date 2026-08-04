#pragma once

#include "CoreMinimal.h"

class UNiagaraComponent;
struct FCk_Particles_StageResult;

// --------------------------------------------------------------------------------------------------------------------
// PER-PART tuning — a fixed-budget block of per-VisTag multipliers a caller may hang on ONE live system instance.
//
// User.CkTuning tunes a whole system with four floats. This tunes the LAYERS inside it: a recreation draws through
// several renderers, each selected by the VisTag its behavior writes, and a designer retuning one layer must not
// move the other twelve. The block is addressed by VisTag, so nothing about it is behavior-specific.
//
// Row addressing: the SHARED renderer set (VisTags 0..ck::particles::SharedRendererVisTag_Max) occupies rows
// 0..4 verbatim, and the behavior's own row-declared band maps onto the rows above it —
// row = (SharedRendererVisTag_Max + 1) + (VisTag - BandStart). BandStart is the behavior's band start VisTag, or 0
// for a behavior that declares no band. A VisTag that lands outside [0, MaxTunedParts) is left UNTUNED rather than
// wrapped, so a mis-stated BandStart cannot silently retune an unrelated layer — and so is one in the open interval
// (SharedRendererVisTag_Max, BandStart), which belongs to no part of the behavior at all.
//
// The budget is a constant, not a tunable: the block is a GPU uniform array, so its size is baked into the shader
// parameter struct. 24 rows covers the widest band on the roster today (16, LightningHit) with headroom.
//
// LOCKSTEP: the layout below is mirrored EXACTLY by CkParticles_ApplyPartTuning / CkParticles_PartTuningRow in
// Shaders/CkParticles/Common.ush and by the shader parameters the DI's template wrapper reads. Same order, same
// math, or the CPU sim and the GPU sim render two different effects.
// --------------------------------------------------------------------------------------------------------------------
namespace ck::particles
{
    inline constexpr int32 MaxTunedParts          = 24;
    inline constexpr int32 PartTuningRowsPerPart  = 5;
    inline constexpr int32 PartTuningFloat4Count  = MaxTunedParts * PartTuningRowsPerPart;
}

// The five float4s one tuned part carries. Every field's IDENTITY is the value that leaves the behavior's own
// output untouched, so a part nobody tuned renders exactly as it does today:
//   R0: SizeMul(1)          StretchMul(1)   MeshScaleMul(1)  SpeedMul(1)
//   R1: TintR(1)            TintG(1)        TintB(1)         AlphaMul(1)
//   R2: DynMulX(1)          DynMulY(1)      DynMulZ(1)       DynMulW(1)   -> per-axis on Out.Dynamic
//   R3: RotationOffsetDeg(0) WindowStart(0) WindowEnd(1)     Unused(0)
//   R4: PosOffsetX(0)       PosOffsetY(0)   PosOffsetZ(0)    Unused(0)
struct CKPARTICLES_API FCkParticles_PartTuningBlock
{
    FCkParticles_PartTuningBlock();

    // The behavior's row-declared band start VisTag; 0 when the behavior declares no band. Phase 2's asset layer
    // computes it from the naming header — this phase takes it as given.
    int32 BandStart = 0;

    FVector4f Rows[ck::particles::PartTuningFloat4Count];

    auto Get_Row(int32 InPart, int32 InRow) const -> const FVector4f&;
    auto Set_Row(int32 InPart, int32 InRow, const FVector4f& InValue) -> void;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles
{
    // Row a VisTag reads, or INDEX_NONE when the tag falls outside the block's budget. Mirrors
    // CkParticles_PartTuningRow (Common.ush).
    CKPARTICLES_API auto Get_PartTuningRowIndex(
        int32 InVisTag,
        int32 InBandStart) -> int32;

    // The CPU mirror of CkParticles_ApplyPartTuning (Common.ush) — the five float4s applied to one stage result,
    // in the same order the GPU applies them.
    CKPARTICLES_API auto Apply_PartTuningRows(
        FCk_Particles_StageResult& InOutResult,
        float                      InNormalizedAge,
        const FVector4f&           InR0,
        const FVector4f&           InR1,
        const FVector4f&           InR2,
        const FVector4f&           InR3,
        const FVector4f&           InR4) -> void;

    // Row lookup + application, the pair the DI's template wrapper performs on the GPU. A result whose VisTag has
    // no row is returned untouched.
    CKPARTICLES_API auto Apply_PartTuning(
        FCk_Particles_StageResult&          InOutResult,
        float                               InNormalizedAge,
        const FCkParticles_PartTuningBlock& InBlock) -> void;

    // ----------------------------------------------------------------------------------------------------------
    // The per-component registry the DI reads its per-instance block from.
    //
    // The block cannot ride a Niagara User parameter (there is no float4-array user type), so it is held module-side
    // and keyed by the component the caller tuned. Entries are weak: a destroyed component's row is dropped on the
    // next sweep rather than keeping the component alive.
    //
    // The revision is a single monotonic counter over the whole registry, not a per-entry one. A DI instance caches
    // the revision it last read and re-fetches only when it moves, so an untouched registry costs one integer
    // compare per instance per tick and uploads nothing.
    // ----------------------------------------------------------------------------------------------------------
    CKPARTICLES_API auto Set_PartTuningBlock(
        const UNiagaraComponent*            InComponent,
        const FCkParticles_PartTuningBlock& InBlock) -> void;

    CKPARTICLES_API auto Remove_PartTuningBlock(
        const UNiagaraComponent* InComponent) -> void;

    // Fills OutBlock with the component's block and answers true; leaves it at the IDENTITY and answers false when
    // the component has no entry. Never leaves OutBlock zeroed — a zero block hides every particle.
    CKPARTICLES_API auto TryGet_PartTuningBlock(
        const UNiagaraComponent*      InComponent,
        FCkParticles_PartTuningBlock& OutBlock) -> bool;

    CKPARTICLES_API auto Get_PartTuningRevision() -> uint64;
}
