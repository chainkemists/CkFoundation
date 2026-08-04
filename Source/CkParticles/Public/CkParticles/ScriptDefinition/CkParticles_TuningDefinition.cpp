#include "CkParticles/ScriptDefinition/CkParticles_TuningDefinition.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"
#include "CkParticles_Log.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkParticles_TuningDefinition)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkParticles_TuningDefinition::
    Get_AsPartTuningBlock(
        int32 InBehaviorId) const
    -> FCkParticles_PartTuningBlock
{
    auto Block = FCkParticles_PartTuningBlock{};
    Block.BandStart = ck::particles::Get_BehaviorBandStart(InBehaviorId);

    for (const auto& Row : _Parts)
    {
        const auto Part = ck::particles::Get_PartTuningRowIndex(Row._VisTag, Block.BandStart);

        if (Part == INDEX_NONE)
        {
            ck::particles::Warning(TEXT("Tuning asset [{}] carries a part row [{}] on VisTag [{}] that behavior [{}] ")
                TEXT("does not declare — skipping it. Re-run Generate Tuning Assets to reconcile the roster."),
                GetName(), Row._PartName, Row._VisTag, InBehaviorId);
            continue;
        }

        Block.Set_Row(Part, 0, FVector4f(
            Row._SizeMultiplier,
            Row._StretchMultiplier,
            Row._MeshScaleMultiplier,
            Row._SpeedMultiplier));

        // The global tint has no slot of its own in User.CkTuning, so it rides every part's tint. Alpha is the
        // row's own on both sides — a colour picked in the wheel must not dim a layer nobody asked to fade.
        Block.Set_Row(Part, 1, FVector4f(
            Row._Tint.R * _GlobalTint.R,
            Row._Tint.G * _GlobalTint.G,
            Row._Tint.B * _GlobalTint.B,
            Row._AlphaMultiplier));

        Block.Set_Row(Part, 2, FVector4f(
            Row._DissolveMultiplier,
            Row._DistortionMultiplier,
            Row._UvPanMultiplier,
            Row._EmissiveMultiplier));

        constexpr auto UnusedSlot = 0.0f;

        Block.Set_Row(Part, 3, FVector4f(
            Row._RotationOffsetDegrees,
            Row._WindowStart,
            Row._WindowEnd,
            UnusedSlot));

        Block.Set_Row(Part, 4, FVector4f(
            static_cast<float>(Row._PositionOffset.X),
            static_cast<float>(Row._PositionOffset.Y),
            static_cast<float>(Row._PositionOffset.Z),
            UnusedSlot));
    }

    return Block;
}

// --------------------------------------------------------------------------------------------------------------------
