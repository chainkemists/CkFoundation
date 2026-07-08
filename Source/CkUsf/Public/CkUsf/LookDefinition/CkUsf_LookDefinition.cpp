#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCkUsf_LookDefinition::
    Get_EffectiveLookName() const
    -> FName
{
    return _LookName.IsNone() ? GetFName() : _LookName;
}

namespace ck_usf_look_definition
{
    // Floats a param consumes when per-instance; 0 = not eligible (textures cannot be per-instance).
    static auto Get_PerInstanceFloatCount(const FCk_Usf_ParamDesc& InParam) -> int32
    {
        if (NOT InParam._PerInstance)
        { return 0; }

        switch (InParam._Type)
        {
            case ECk_Usf_ParamType::Scalar: return 1;
            case ECk_Usf_ParamType::Vector: return 3;
            default:                        return 0;
        }
    }
}

auto
    UCkUsf_LookDefinition::
    Get_PerInstanceSlotOf(
        const FName InParamName) const
    -> int32
{
    auto Slot = 0;
    for (const auto& Param : _Parameters)
    {
        const auto NumFloats = ck_usf_look_definition::Get_PerInstanceFloatCount(Param);
        if (NumFloats == 0)
        { continue; }

        if (Param._Name == InParamName)
        { return (Param._PerInstanceSlot >= 0) ? Param._PerInstanceSlot : Slot; }

        // Explicit slots live outside the auto layout — they do not advance the counter.
        if (Param._PerInstanceSlot < 0)
        { Slot += NumFloats; }
    }
    return INDEX_NONE;
}

auto
    UCkUsf_LookDefinition::
    Get_NumPerInstanceFloats() const
    -> int32
{
    // Allocation must cover the auto layout AND the end of the farthest explicit slot —
    // an explicit-slot param (e.g. batched-crowd floats [10..12]) can sit past the auto total.
    auto AutoTotal = 0;
    auto MaxExplicitEnd = 0;
    for (const auto& Param : _Parameters)
    {
        const auto NumFloats = ck_usf_look_definition::Get_PerInstanceFloatCount(Param);
        if (NumFloats == 0)
        { continue; }

        if (Param._PerInstanceSlot >= 0)
        { MaxExplicitEnd = FMath::Max(MaxExplicitEnd, Param._PerInstanceSlot + NumFloats); }
        else
        { AutoTotal += NumFloats; }
    }
    return FMath::Max(AutoTotal, MaxExplicitEnd);
}

// --------------------------------------------------------------------------------------------------------------------
