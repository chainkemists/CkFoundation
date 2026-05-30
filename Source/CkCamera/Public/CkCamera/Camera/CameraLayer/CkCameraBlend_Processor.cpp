#include "CkCameraBlend_Processor.h"

#include "CkCamera/Camera/CameraLayer/EntityScripts/CkCameraLayer_EntityScript.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"
#include "CkEcs/Handle/CkHandle_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkAttribute/FloatAttribute/CkFloatAttribute_Utils.h"
#include "CkAttribute/VectorAttribute/CkVectorAttribute_Utils.h"
#include "CkAttribute/RotatorAttribute/CkRotatorAttribute_Utils.h"
#include "CkAttribute/IntegerAttribute/CkIntegerAttribute_Utils.h"

CK_REGISTER_PROCESSOR(ck::FProcessor_CameraLayer_Blend);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CameraLayer_Blend::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InLayer,
            FFragment_CameraLayer_Blend& InBlend,
            FFragment_CameraLayer_AcquiredModifiers& InAcquired)
        -> void
    {
        const auto DeltaSeconds = static_cast<float>(InDeltaT.Get_Seconds());

        // Advance the blend weight toward its target (this is the SOLE place alpha advances).
        InBlend._Alpha = FMath::FInterpConstantTo(
            InBlend._Alpha, InBlend._TargetAlpha, DeltaSeconds, InBlend._BlendRate);

        const auto Alpha = InBlend._Alpha;

        // ---- Float ----
        for (auto& Entry : InAcquired._Float)
        {
            if (ck::Is_NOT_Valid(Entry._Modifier))
            { continue; }

            auto Effective = 0.0f;
            switch (Entry._Op)
            {
                case ECk_AttributeModifier_Operation::Multiply:
                {
                    Effective = FMath::Lerp(1.0f, Entry._Target, Alpha);
                    break;
                }
                case ECk_AttributeModifier_Operation::Override:
                {
                    const auto Base = UCk_Utils_FloatAttribute_UE::Get_BaseValue(Entry._Attribute, Entry._Component);
                    Effective = (Entry._Target - Base) * Alpha;
                    break;
                }
                default:
                {
                    Effective = Entry._Target * Alpha;
                    break;
                }
            }

            UCk_Utils_FloatAttributeModifier_UE::Override(Entry._Modifier, Effective);
        }

        // ---- Vector ----
        for (auto& Entry : InAcquired._Vector)
        {
            if (ck::Is_NOT_Valid(Entry._Modifier))
            { continue; }

            auto Effective = FVector::ZeroVector;
            switch (Entry._Op)
            {
                case ECk_AttributeModifier_Operation::Multiply:
                {
                    Effective = FMath::Lerp(FVector::OneVector, Entry._Target, Alpha);
                    break;
                }
                case ECk_AttributeModifier_Operation::Override:
                {
                    const auto Base = UCk_Utils_VectorAttribute_UE::Get_BaseValue(Entry._Attribute, Entry._Component);
                    Effective = (Entry._Target - Base) * Alpha;
                    break;
                }
                default:
                {
                    Effective = Entry._Target * Alpha;
                    break;
                }
            }

            UCk_Utils_VectorAttributeModifier_UE::Override(Entry._Modifier, Effective);
        }

        // ---- Rotator ----
        for (auto& Entry : InAcquired._Rotator)
        {
            if (ck::Is_NOT_Valid(Entry._Modifier))
            { continue; }

            auto Effective = FRotator::ZeroRotator;
            switch (Entry._Op)
            {
                case ECk_AttributeModifier_Operation::Multiply:
                {
                    const auto One = FRotator{1.0f, 1.0f, 1.0f};
                    Effective = One + (Entry._Target - One) * Alpha;
                    break;
                }
                case ECk_AttributeModifier_Operation::Override:
                {
                    const auto Base = UCk_Utils_RotatorAttribute_UE::Get_BaseValue(Entry._Attribute, Entry._Component);
                    Effective = (Entry._Target - Base) * Alpha;
                    break;
                }
                default:
                {
                    Effective = Entry._Target * Alpha;
                    break;
                }
            }

            UCk_Utils_RotatorAttributeModifier_UE::Override(Entry._Modifier, Effective);
        }

        // ---- Integer ----
        for (auto& Entry : InAcquired._Integer)
        {
            if (ck::Is_NOT_Valid(Entry._Modifier))
            { continue; }

            const auto Target = static_cast<float>(Entry._Target);
            auto Effective = 0;
            switch (Entry._Op)
            {
                case ECk_AttributeModifier_Operation::Multiply:
                {
                    Effective = FMath::RoundToInt(FMath::Lerp(1.0f, Target, Alpha));
                    break;
                }
                case ECk_AttributeModifier_Operation::Override:
                {
                    const auto Base = static_cast<float>(UCk_Utils_IntegerAttribute_UE::Get_BaseValue(Entry._Attribute, Entry._Component));
                    Effective = FMath::RoundToInt((Target - Base) * Alpha);
                    break;
                }
                default:
                {
                    Effective = FMath::RoundToInt(Target * Alpha);
                    break;
                }
            }

            UCk_Utils_IntegerAttributeModifier_UE::Override(Entry._Modifier, Effective);
        }

        // Hand the current alpha to the layer script for any custom per-frame logic (standard tuner blending above
        // is automatic; this is the opt-in hook the layer can override).
        if (auto* Script = ::Cast<UCk_CameraLayer_EntityScript>(
                InLayer.Get<ck::FFragment_EntityScript_Current>().Get_Script().Get());
            ck::IsValid(Script))
        {
            Script->Blend(InLayer, Alpha);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
