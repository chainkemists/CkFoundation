#include "CkProceduralAnimation/BodyPose/CkProceduralBodyPose_Processor.h"

#include "CkProceduralAnimation/Core/CkProceduralBodySupport.h"
#include "CkProceduralAnimation/Leg/CkProceduralLeg_Fragment.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include <Kismet/KismetMathLibrary.h>

CK_REGISTER_PROCESSOR(ck::FProcessor_ProceduralBodyPose_Update);

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_body_pose
{
    // A leg that is gone or going keeps its captured slot at weight zero; its placement is unreadable once destroyed,
    // so it sits at the body origin.
    auto
        Get_SupportLeg(
            const FCk_Handle_ProceduralLeg& InLeg)
        -> ck::FProceduralBodySupportLeg
    {
        constexpr auto Unsupported = 0.0f;
        constexpr auto Supporting = 1.0f;

        if (ck::Is_NOT_Valid(InLeg) || InLeg.Has<ck::FTag_DestroyEntity_Initiate>())
        { return ck::FProceduralBodySupportLeg{FVector::ZeroVector, Unsupported}; }

        const auto& HipLocal = InLeg.Get<ck::FFragment_ProceduralLeg_Params>().Get_Placement().Get_HipLocal();
        const auto Weight = InLeg.Has<ck::FTag_ProceduralLeg_Disabled>() ? Unsupported : Supporting;
        return ck::FProceduralBodySupportLeg{HipLocal, Weight};
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_ProceduralBodyPose_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProceduralBodyPose_Params& InParams,
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

        const auto Legs = algo::Transform<TArray<FProceduralBodySupportLeg, TInlineAllocator<8>>>(
            InGaitComp._Legs, &ck_procedural_body_pose::Get_SupportLeg);
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
