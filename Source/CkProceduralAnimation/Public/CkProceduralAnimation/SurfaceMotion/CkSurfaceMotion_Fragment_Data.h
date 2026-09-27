#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkJolt/Query/CkJoltQuery_Data.h"

#include "CkSurfaceMotion_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_SurfaceMotion : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_SurfaceMotion);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SurfaceMotion);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_SurfaceMotion_Support : uint8
{
    Grounded,
    Airborne
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SurfaceMotion_Support);

UENUM(BlueprintType)
enum class ECk_SurfaceMotion_ContactQuery : uint8
{
    Trusted,
    Missed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SurfaceMotion_ContactQuery);

// The ray whose hit the body last accepted: forward within the clearance, down under the body, the look-ahead down ray
// ahead of the body, the fan around a convex edge, or the swept fall; Feet, the plane through the planted feet (height
// source PlantedFeet); or Step, the top of a face the body steps onto (max step height). None while nothing supports the
// body. A substep that coasts while a large turn waits for confirmation accepts nothing and keeps the source.
UENUM(BlueprintType)
enum class ECk_SurfaceMotion_ContactSource : uint8
{
    None,
    Forward,
    Down,
    LookAhead,
    Fan,
    Fall,
    Feet,
    Step
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SurfaceMotion_ContactSource);

UENUM(BlueprintType)
enum class ECk_SurfaceMotion_WallPolicy : uint8
{
    Climb  UMETA(DisplayName = "Climb (a face within the clearance becomes the next support)"),
    Slide  UMETA(DisplayName = "Slide (the body slides along a face it cannot step onto)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SurfaceMotion_WallPolicy);

// Wall while the last substep slid the body along a face it could neither step onto nor climb: every wall under Slide, and a
// face without room under either policy.
UENUM(BlueprintType)
enum class ECk_SurfaceMotion_Obstruction : uint8
{
    None,
    Wall
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SurfaceMotion_Obstruction);

UENUM(BlueprintType)
enum class ECk_SurfaceMotion_HeightSource : uint8
{
    Rays         UMETA(DisplayName = "Rays (the body keeps its clearance above what its rays hit)"),
    PlantedFeet  UMETA(DisplayName = "Planted Feet (the body also rides on the plane through its planted feet)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SurfaceMotion_HeightSource);

UENUM(BlueprintType)
enum class ECk_SurfaceMotion_Failure : uint8
{
    None,
    NanBody
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_SurfaceMotion_Failure);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_SurfaceMotion_Contact
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SurfaceMotion_Contact);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Clearance = 65.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ProbeReach = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _ContactGrace = FCk_Time{0.12};

    // A contact whose normal turns more than this many degrees from the support is adopted only after it has been seen,
    // each sighting within 15 degrees of the last, for _ConfirmTime while the support under the body still holds. The
    // body keeps walking meanwhile, so a face it only grazes is never adopted, and a body longer than its clearance can
    // overrun a head-on wall by up to its speed times _ConfirmTime.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 180))
    float _ConfirmAngle = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _ConfirmTime = FCk_Time{0.075};

    // PlantedFeet also keeps the body at its clearance above the plane the body's gait fits through the ground its feet
    // stand on and are about to land on, within their footprint, whenever that plane lies above what the rays hit; the rays
    // decide attitude where the down ray hits and the plane's normal over a down miss, and outside the footprint only the
    // rays count. A body without a gait has no such plane.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_SurfaceMotion_HeightSource _HeightSource = ECk_SurfaceMotion_HeightSource::Rays;

    // 0: no stepping. Otherwise above the clearance and at most the probe reach: a face the forward ray meets (one taller
    // than the clearance) whose top lies at most this high above the ground under the body is a step, and the body rises
    // onto its top instead of treating it as a wall. A face lower than the clearance is never seen by the forward ray; the
    // down ray lifts the body over it whatever this says.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    float _MaxStepHeight = 0.0f;

    // What a face that is not a step is: Climb confirms it and makes it the next support; Slide never makes it support and
    // takes the travel into it away, so a glancing face slows the body and a head-on one stops it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_SurfaceMotion_WallPolicy _WallPolicy = ECk_SurfaceMotion_WallPolicy::Climb;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Jolt_QueryFilter _QueryFilter;

public:
    CK_PROPERTY(_Clearance);
    CK_PROPERTY(_ProbeReach);
    CK_PROPERTY(_ContactGrace);
    CK_PROPERTY(_ConfirmAngle);
    CK_PROPERTY(_ConfirmTime);
    CK_PROPERTY(_HeightSource);
    CK_PROPERTY(_MaxStepHeight);
    CK_PROPERTY(_WallPolicy);
    CK_PROPERTY(_QueryFilter);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_SurfaceMotion_Contact, IsValid_Policy_Default,
[=](const FCk_SurfaceMotion_Contact& InContact)
{
    return FMath::IsFinite(InContact.Get_Clearance()) && InContact.Get_Clearance() > 0.0f
        && FMath::IsFinite(InContact.Get_ProbeReach()) && InContact.Get_ProbeReach() > InContact.Get_Clearance()
        && FMath::IsFinite(InContact.Get_ContactGrace().Get_Seconds()) && InContact.Get_ContactGrace() >= FCk_Time{}
        && FMath::IsFinite(InContact.Get_ConfirmAngle()) && InContact.Get_ConfirmAngle() >= 0.0f && InContact.Get_ConfirmAngle() <= 180.0f
        && FMath::IsFinite(InContact.Get_ConfirmTime().Get_Seconds()) && InContact.Get_ConfirmTime() >= FCk_Time{}
        && FMath::IsFinite(InContact.Get_MaxStepHeight())
        && (InContact.Get_MaxStepHeight() == 0.0f
            || (InContact.Get_MaxStepHeight() > InContact.Get_Clearance() && InContact.Get_MaxStepHeight() <= InContact.Get_ProbeReach()));
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_SurfaceMotion_Movement
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SurfaceMotion_Movement);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _MaxSpeed = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _SurfaceTurnRate = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _ClearanceSpeed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _Gravity = FVector{0.0, 0.0, -980.0};

    // While the steering direction's projection onto the support plane is shorter than this fraction of the direction,
    // the body keeps its travel tangent: on a face nearly perpendicular to the steering, the projection is a small vector
    // whose direction flips with every facet.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0, ClampMax = 1))
    float _SteerFloor = 0.4f;

public:
    CK_PROPERTY(_MaxSpeed);
    CK_PROPERTY(_SurfaceTurnRate);
    CK_PROPERTY(_ClearanceSpeed);
    CK_PROPERTY(_Gravity);
    CK_PROPERTY(_SteerFloor);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_SurfaceMotion_Movement, IsValid_Policy_Default,
[=](const FCk_SurfaceMotion_Movement& InMovement)
{
    return FMath::IsFinite(InMovement.Get_MaxSpeed()) && InMovement.Get_MaxSpeed() > 0.0f
        && FMath::IsFinite(InMovement.Get_SurfaceTurnRate()) && InMovement.Get_SurfaceTurnRate() > 0.0f
        && FMath::IsFinite(InMovement.Get_ClearanceSpeed()) && InMovement.Get_ClearanceSpeed() > 0.0f
        && NOT InMovement.Get_Gravity().ContainsNaN()
        && FMath::IsFinite(InMovement.Get_SteerFloor()) && InMovement.Get_SteerFloor() >= 0.0f && InMovement.Get_SteerFloor() <= 1.0f;
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_SurfaceMotion_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SurfaceMotion_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_SurfaceMotion_Contact _Contact;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_SurfaceMotion_Movement _Movement;

public:
    CK_PROPERTY(_Contact);
    CK_PROPERTY(_Movement);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_SurfaceMotion_Spec, IsValid_Policy_Default,
[=](const FCk_SurfaceMotion_Spec& InParams)
{
    return ck::IsValid(InParams.Get_Contact()) && ck::IsValid(InParams.Get_Movement());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Request_SurfaceMotion_Steering : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_SurfaceMotion_Steering);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_SurfaceMotion_Steering);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _WorldDirection = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Speed = 0.0f;

public:
    CK_PROPERTY(_WorldDirection);
    CK_PROPERTY(_Speed);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_SurfaceMotion_Steering, _WorldDirection, _Speed);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Request_SurfaceMotion_Steering, IsValid_Policy_Default,
[=](const FCk_Request_SurfaceMotion_Steering& InRequest)
{
    return NOT InRequest.Get_WorldDirection().ContainsNaN()
        && FMath::IsFinite(InRequest.Get_Speed()) && InRequest.Get_Speed() >= 0.0f
        && (InRequest.Get_Speed() == 0.0f || NOT InRequest.Get_WorldDirection().IsNearlyZero());
});

// --------------------------------------------------------------------------------------------------------------------
