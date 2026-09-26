#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Processor.h"

#include "CkProceduralAnimation/Core/CkProceduralBodySupport.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <Kismet/KismetMathLibrary.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralBodyPose_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralBodyPose_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralBodyPose_Params& InParams,
            const FFragment_ProceduralBodyPose_SupportLayout& InSupportLayout,
            FFragment_ProceduralBodyPose& InPoseComp,
            const FFragment_ProceduralGait& InGaitComp,
            const FFragment_Transform& InTransform)
        -> void
    {
        const auto DeltaSeconds = static_cast<float>(InDeltaT.Get_Seconds());
        if (NOT FMath::IsFinite(DeltaSeconds) || DeltaSeconds <= 0.0f)
        { return; }

        auto Presentation = InParams.Get_Presentation();
        const auto PresentationLive = ck::IsValid(Presentation)
            && NOT Presentation.Has<FTag_DestroyEntity_Initiate>()
            && UCk_Utils_Transform_UE::Has(Presentation);
        CK_ENSURE_IF_NOT(PresentationLive,
            TEXT("Procedural body pose [{}] lost its presentation entity; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MissingPresentation);
            return;
        }

        const auto& HipLocals = InSupportLayout.Get_HipLocals();
        const auto LayoutValid = HipLocals.Num() == InGaitComp._Legs.Num();
        CK_ENSURE_IF_NOT(LayoutValid,
            TEXT("Procedural body pose [{}] captured [{}] hips for [{}] gait legs; feature is failed."),
            InHandle, HipLocals.Num(), InGaitComp._Legs.Num())
        {
            InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MalformedSupport);
            return;
        }

        constexpr auto Unsupported = 0.0f;
        constexpr auto Supporting = 1.0f;

        auto Legs = TArray<FProceduralBodySupportLeg, TInlineAllocator<8>>{};
        Legs.Reserve(HipLocals.Num());
        for (auto Index = 0; Index < HipLocals.Num(); ++Index)
        {
            const auto& Leg = InGaitComp._Legs[Index];
            const auto Supports = ck::IsValid(Leg)
                && NOT Leg.Has<FTag_DestroyEntity_Initiate>()
                && NOT Leg.Has<FTag_ProceduralLeg_Disabled>();
            Legs.Emplace(HipLocals[Index], Supports ? Supporting : Unsupported);
        }

        const auto& Support = InParams.Get_Support();
        const auto Settings = FProceduralBodySupportSettings{}
            .Set_CollapseDrop(Support.Get_CollapseDrop())
            .Set_MaxTiltDegrees(Support.Get_MaxTilt());

        const auto Target = ComputeProceduralBodySupportPose(Legs, Settings);
        const auto TargetValid = Target.IsSet();
        CK_ENSURE_IF_NOT(TargetValid,
            TEXT("Procedural body pose [{}] support input was malformed; feature is failed."), InHandle)
        {
            InHandle.Add<FFragment_ProceduralBodyPose_Failure>(ECk_ProceduralBodyPose_Failure::MalformedSupport);
            return;
        }

        const auto& Spring = InParams.Get_Spring();
        const auto Location = UKismetMathLibrary::VectorSpringInterp(InPoseComp._Offset.GetLocation(), Target->GetLocation(),
            InPoseComp._TranslationSpring, Spring.Get_Stiffness(), Spring.Get_CriticalDampingFactor(), DeltaSeconds, Spring.Get_Mass());
        const auto Rotation = UKismetMathLibrary::QuaternionSpringInterp(InPoseComp._Offset.GetRotation(), Target->GetRotation(),
            InPoseComp._RotationSpring, Spring.Get_Stiffness(), Spring.Get_CriticalDampingFactor(), DeltaSeconds, Spring.Get_Mass());
        InPoseComp._Offset = FTransform{Rotation.GetNormalized(), Location};

        const auto Posed = InPoseComp._Offset * InTransform.Get_Transform();
        UCk_Utils_Transform_UE::Request_SetTransform(Presentation, FCk_Request_Transform_SetTransform{Posed}, {});
    }
}

// --------------------------------------------------------------------------------------------------------------------
