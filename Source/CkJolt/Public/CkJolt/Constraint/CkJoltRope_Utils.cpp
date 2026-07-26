#include "CkJoltRope_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkJolt/Body/CkJoltBody_Utils.h"
#include "CkJolt/CkJolt_Log.h"
#include "CkJolt/Constraint/CkJoltConstraint_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_JoltRope_UE::
    Create_Rope(
        FCk_Handle& InOwner,
        const FCk_JoltRope_ParamsData& InParams)
    -> FCk_JoltRope_Result
{
    CK_ENSURE_IF_NOT(ck::IsValid(InOwner),
        TEXT("Invalid Owner Handle passed to Create_Rope"))
    { return {}; }

    CK_ENSURE_IF_NOT(NOT InParams.Get_Direction().IsNearlyZero(),
        TEXT("Create_Rope: Direction is (near) zero"))
    { return {}; }

    const auto SegmentCount = FMath::Clamp(InParams.Get_SegmentCount(), 1, 200);
    const auto SegmentLength = InParams.Get_SegmentLength();
    const auto Direction = InParams.Get_Direction().GetSafeNormal();
    const auto& Anchor = InParams.Get_AnchorLocation();

    const auto AnchorBody = InParams.Get_AnchorBody();
    const auto AnchorToWorld = ck::Is_NOT_Valid(AnchorBody);

    if (NOT AnchorToWorld)
    {
        CK_ENSURE_IF_NOT(UCk_Utils_JoltBody_UE::Has(AnchorBody),
            TEXT("Create_Rope: AnchorBody [{}] has NO JoltBody feature"), AnchorBody)
        { return {}; }
    }

    auto Segments = TArray<FCk_Handle_JoltBody>{};
    auto Links = TArray<FCk_Handle_JoltConstraint>{};
    Segments.Reserve(SegmentCount);
    Links.Reserve(SegmentCount);

    for (auto Index = 0; Index < SegmentCount; ++Index)
    {
        const auto Center = Anchor + Direction * (SegmentLength * (static_cast<float>(Index) + 0.5f));

        auto SegmentEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InOwner, [&](FCk_Handle InNewEntity)
        {
#if NOT CK_DISABLE_ECS_HANDLE_DEBUGGING
            UCk_Utils_Handle_UE::Set_DebugName(InNewEntity,
                *ck::Format_UE(TEXT("JoltRope Segment [{}]"), Index));
#endif
        });

        UCk_Utils_Transform_UE::Add(SegmentEntity, FTransform{Center}, ECk_Replication::DoesNotReplicate);

        const auto BodyParams = FCk_Fragment_JoltBody_ParamsData{ECk_JoltBody_ShapeSource::ExplicitShape}
            .Set_ShapeDimensions(FCk_Jolt_ShapeDimensions{ECk_Jolt_ShapeType::Sphere}
                .Set_Radius(InParams.Get_SegmentRadius()))
            .Set_MotionType(ECk_MotionType::Dynamic)
            .Set_MassSource(ECk_JoltBody_MassSource::Explicit)
            .Set_MassKg(InParams.Get_SegmentMassKg())
            .Set_LinearDamping(InParams.Get_LinearDamping())
            .Set_AngularDamping(InParams.Get_AngularDamping())
            .Set_GravityFactor(InParams.Get_GravityFactor())
            .Set_CollisionProfileName(InParams.Get_CollisionProfileName());

        auto SegmentBody = UCk_Utils_JoltBody_UE::Add(SegmentEntity, BodyParams);

        CK_ENSURE_IF_NOT(ck::IsValid(SegmentBody),
            TEXT("Create_Rope: segment [{}] JoltBody Add FAILED — returning the partial rope"), Index)
        { return FCk_JoltRope_Result{Segments}.Set_Links(Links); }

        auto PreviousBody = AnchorBody;
        if (Index > 0)
        { PreviousBody = static_cast<FCk_Handle>(Segments.Last()); }

        auto ConstraintParams = FCk_Fragment_JoltConstraint_ParamsData{};

        switch (InParams.Get_LinkMode())
        {
            case ECk_JoltRope_LinkMode::Rigid:
            {
                const auto Boundary = Anchor + Direction * (SegmentLength * static_cast<float>(Index));

                ConstraintParams = FCk_Fragment_JoltConstraint_ParamsData{ECk_JoltConstraint_Type::Point}
                    .Set_OtherBody(PreviousBody)
                    .Set_WorldAnchorA(Boundary)
                    .Set_WorldAnchorB(Boundary);
                break;
            }
            case ECk_JoltRope_LinkMode::Springy:
            {
                const auto PreviousPoint = Index == 0
                    ? Anchor
                    : Anchor + Direction * (SegmentLength * (static_cast<float>(Index) - 0.5f));

                ConstraintParams = FCk_Fragment_JoltConstraint_ParamsData{ECk_JoltConstraint_Type::Distance}
                    .Set_OtherBody(PreviousBody)
                    .Set_WorldAnchorA(Center)
                    .Set_WorldAnchorB(PreviousPoint)
                    .Set_UseSpring(ECk_EnableDisable::Enable)
                    .Set_SpringMode(ECk_JoltConstraint_SpringMode::FrequencyAndDamping)
                    .Set_SpringFrequencyOrStiffness(InParams.Get_SpringFrequency())
                    .Set_SpringDamping(InParams.Get_SpringDamping());
                break;
            }
        }

        auto Link = UCk_Utils_JoltConstraint_UE::Create(SegmentBody, ConstraintParams);

        CK_ENSURE_IF_NOT(ck::IsValid(Link),
            TEXT("Create_Rope: segment [{}] link Create FAILED — returning the partial rope"), Index)
        { return FCk_JoltRope_Result{Segments}.Set_Links(Links); }

        Segments.Emplace(SegmentBody);
        Links.Emplace(Link);
    }

    return FCk_JoltRope_Result{Segments}.Set_Links(Links);
}

// --------------------------------------------------------------------------------------------------------------------
