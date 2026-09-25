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

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Jolt_QueryFilter _QueryFilter;

public:
    CK_PROPERTY(_Clearance);
    CK_PROPERTY(_ProbeReach);
    CK_PROPERTY(_ContactGrace);
    CK_PROPERTY(_QueryFilter);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_SurfaceMotion_Contact, IsValid_Policy_Default,
[=](const FCk_SurfaceMotion_Contact& InContact)
{
    return FMath::IsFinite(InContact.Get_Clearance()) && InContact.Get_Clearance() > 0.0f
        && FMath::IsFinite(InContact.Get_ProbeReach()) && InContact.Get_ProbeReach() > InContact.Get_Clearance()
        && FMath::IsFinite(InContact.Get_ContactGrace().Get_Seconds()) && InContact.Get_ContactGrace() >= FCk_Time{};
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

public:
    CK_PROPERTY(_MaxSpeed);
    CK_PROPERTY(_SurfaceTurnRate);
    CK_PROPERTY(_ClearanceSpeed);
    CK_PROPERTY(_Gravity);
};

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_SurfaceMotion_Movement, IsValid_Policy_Default,
[=](const FCk_SurfaceMotion_Movement& InMovement)
{
    return FMath::IsFinite(InMovement.Get_MaxSpeed()) && InMovement.Get_MaxSpeed() > 0.0f
        && FMath::IsFinite(InMovement.Get_SurfaceTurnRate()) && InMovement.Get_SurfaceTurnRate() > 0.0f
        && FMath::IsFinite(InMovement.Get_ClearanceSpeed()) && InMovement.Get_ClearanceSpeed() > 0.0f
        && NOT InMovement.Get_Gravity().ContainsNaN();
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Fragment_SurfaceMotion_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_SurfaceMotion_ParamsData);

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

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Fragment_SurfaceMotion_ParamsData, IsValid_Policy_Default,
[=](const FCk_Fragment_SurfaceMotion_ParamsData& InParams)
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
