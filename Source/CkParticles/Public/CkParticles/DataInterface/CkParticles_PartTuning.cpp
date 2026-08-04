#include "CkParticles/DataInterface/CkParticles_PartTuning.h"

#include "CkParticles/DataInterface/CkParticles_DataInterface.h"
#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "CkCore/Macros/CkMacros.h"

#include "Misc/ScopeLock.h"

#include "NiagaraComponent.h"

// The shared-set fast path in Get_PartTuningRowIndex (and its .ush mirror) returns VisTags 0..SharedRendererVisTag_Max
// as row indices UNCHECKED — a budget at or below the shared set overruns the stack the block lives on.
static_assert(ck::particles::MaxTunedParts > ck::particles::SharedRendererVisTag_Max,
    "MaxTunedParts must exceed SharedRendererVisTag_Max: shared VisTags index block rows unchecked.");

// --------------------------------------------------------------------------------------------------------------------
// Filename-derived namespace rather than an anonymous one: unity builds fuse anonymous namespaces across the
// concatenated TUs and same-named module state collides.
namespace ck_particles_part_tuning
{
    FCriticalSection Lock;

    using FKey = TWeakObjectPtr<UNiagaraComponent>;

    TMap<FKey, FCkParticles_PartTuningBlock> Blocks;

    // Starts at 1 so a DI instance's cached revision of 0 always reads as stale on its first tick, which is what
    // makes the very first fetch happen without a special case for it.
    uint64 Revision = 1;

    // Dropping rows whose component has gone is the only reclamation this registry has: a stale weak key never
    // matches a live component again, so an unswept map would grow for the lifetime of the process.
    auto DoDropStaleEntries() -> void
    {
        for (auto It = Blocks.CreateIterator(); It; ++It)
        {
            if (NOT It.Key().IsValid())
            { It.RemoveCurrent(); }
        }
    }

    auto Make_Key(const UNiagaraComponent* InComponent) -> FKey
    {
        return FKey{const_cast<UNiagaraComponent*>(InComponent)};
    }
}

// --------------------------------------------------------------------------------------------------------------------

FCkParticles_PartTuningBlock::
    FCkParticles_PartTuningBlock()
{
    for (auto Part = 0; Part < ck::particles::MaxTunedParts; ++Part)
    {
        const auto Base = Part * ck::particles::PartTuningRowsPerPart;

        Rows[Base + 0] = FVector4f(1.0f, 1.0f, 1.0f, 1.0f); // SizeMul, StretchMul, MeshScaleMul, SpeedMul
        Rows[Base + 1] = FVector4f(1.0f, 1.0f, 1.0f, 1.0f); // TintR, TintG, TintB, AlphaMul
        Rows[Base + 2] = FVector4f(1.0f, 1.0f, 1.0f, 1.0f); // DynMul XYZW
        Rows[Base + 3] = FVector4f(0.0f, 0.0f, 1.0f, 0.0f); // RotationOffsetDeg, WindowStart, WindowEnd, unused
        Rows[Base + 4] = FVector4f(0.0f, 0.0f, 0.0f, 0.0f); // PosOffset XYZ, unused
    }
}

auto
    FCkParticles_PartTuningBlock::
    Get_Row(
        int32 InPart,
        int32 InRow) const
    -> const FVector4f&
{
    return Rows[InPart * ck::particles::PartTuningRowsPerPart + InRow];
}

auto
    FCkParticles_PartTuningBlock::
    Set_Row(
        int32            InPart,
        int32            InRow,
        const FVector4f& InValue)
    -> void
{
    Rows[InPart * ck::particles::PartTuningRowsPerPart + InRow] = InValue;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::particles::
    Get_PartTuningRowIndex(
        int32 InVisTag,
        int32 InBandStart)
    -> int32
{
    if (InVisTag <= SharedRendererVisTag_Max)
    { return InVisTag; }

    // Above the shared set but BELOW the behavior's own band: the tag belongs to no part of this behavior. Without
    // this the band arithmetic runs backwards and lands inside rows 0..4, silently retuning a shared layer the
    // caller never addressed.
    if (InVisTag < InBandStart)
    { return INDEX_NONE; }

    const auto Row = (SharedRendererVisTag_Max + 1) + (InVisTag - InBandStart);

    return Row >= 0 && Row < MaxTunedParts ? Row : INDEX_NONE;
}

auto
    ck::particles::
    Apply_PartTuningRows(
        FCk_Particles_StageResult& InOutResult,
        float                      InNormalizedAge,
        const FVector4f&           InR0,
        const FVector4f&           InR1,
        const FVector4f&           InR2,
        const FVector4f&           InR3,
        const FVector4f&           InR4)
    -> void
{
    InOutResult.Size     *= InR0.X;
    InOutResult.Size.Y   *= InR0.Y;
    InOutResult.Scale    *= InR0.Z;
    InOutResult.Velocity *= InR0.W;

    InOutResult.Color.R *= InR1.X;
    InOutResult.Color.G *= InR1.Y;
    InOutResult.Color.B *= InR1.Z;
    InOutResult.Color.A *= InR1.W;

    InOutResult.Dynamic *= InR2;

    InOutResult.Rotation += InR3.X;

    // Visibility window — a HARD hide outside [start, end], not a fade: a layer a designer windowed out must
    // contribute nothing, and a scaled-down sprite still writes depth and still costs a draw.
    if (InNormalizedAge < InR3.Y || InNormalizedAge > InR3.Z)
    {
        InOutResult.Color.A = 0.0f;
        InOutResult.Size    = FVector2f::ZeroVector;
        InOutResult.Scale   = FVector3f::ZeroVector;
    }

    InOutResult.Position += FVector3f(InR4.X, InR4.Y, InR4.Z);
}

auto
    ck::particles::
    Apply_PartTuning(
        FCk_Particles_StageResult&          InOutResult,
        float                               InNormalizedAge,
        const FCkParticles_PartTuningBlock& InBlock)
    -> void
{
    const auto Part = Get_PartTuningRowIndex(InOutResult.VisTag, InBlock.BandStart);

    if (Part == INDEX_NONE)
    { return; }

    Apply_PartTuningRows(InOutResult, InNormalizedAge,
        InBlock.Get_Row(Part, 0),
        InBlock.Get_Row(Part, 1),
        InBlock.Get_Row(Part, 2),
        InBlock.Get_Row(Part, 3),
        InBlock.Get_Row(Part, 4));
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::particles::
    Set_PartTuningBlock(
        const UNiagaraComponent*            InComponent,
        const FCkParticles_PartTuningBlock& InBlock)
    -> void
{
    if (NOT IsValid(InComponent))
    { return; }

    auto Guard = FScopeLock{&ck_particles_part_tuning::Lock};

    ck_particles_part_tuning::DoDropStaleEntries();
    ck_particles_part_tuning::Blocks.Add(ck_particles_part_tuning::Make_Key(InComponent), InBlock);
    ++ck_particles_part_tuning::Revision;
}

auto
    ck::particles::
    Remove_PartTuningBlock(
        const UNiagaraComponent* InComponent)
    -> void
{
    auto Guard = FScopeLock{&ck_particles_part_tuning::Lock};

    ck_particles_part_tuning::DoDropStaleEntries();
    ck_particles_part_tuning::Blocks.Remove(ck_particles_part_tuning::Make_Key(InComponent));
    ++ck_particles_part_tuning::Revision;
}

auto
    ck::particles::
    TryGet_PartTuningBlock(
        const UNiagaraComponent*      InComponent,
        FCkParticles_PartTuningBlock& OutBlock)
    -> bool
{
    OutBlock = FCkParticles_PartTuningBlock{};

    if (NOT IsValid(InComponent))
    { return false; }

    auto Guard = FScopeLock{&ck_particles_part_tuning::Lock};

    const auto* Found = ck_particles_part_tuning::Blocks.Find(ck_particles_part_tuning::Make_Key(InComponent));

    if (Found == nullptr)
    { return false; }

    OutBlock = *Found;
    return true;
}

auto
    ck::particles::
    Get_PartTuningRevision()
    -> uint64
{
    auto Guard = FScopeLock{&ck_particles_part_tuning::Lock};
    return ck_particles_part_tuning::Revision;
}
