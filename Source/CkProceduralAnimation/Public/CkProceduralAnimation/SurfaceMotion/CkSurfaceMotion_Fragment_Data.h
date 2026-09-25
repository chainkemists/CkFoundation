#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkJolt/Query/CkJoltQuery_Data.h"

#include "CkSurfaceMotion_Fragment_Data.generated.h"

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKPROCEDURALANIMATION_API FCk_Handle_SurfaceMotion : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_SurfaceMotion);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_SurfaceMotion);

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Fragment_SurfaceMotion_ParamsData
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Fragment_SurfaceMotion_ParamsData);

private:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Clearance = 65.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ProbeReach = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _MaxSpeed = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _SurfaceTurnRate = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _ClearanceSpeed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Time _ContactGrace = FCk_Time{0.12};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _Gravity = FVector{0.0, 0.0, -980.0};

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Jolt_QueryFilter _QueryFilter;
public:
    CK_PROPERTY(_Clearance);
    CK_PROPERTY(_ProbeReach);
    CK_PROPERTY(_MaxSpeed);
    CK_PROPERTY(_SurfaceTurnRate);
    CK_PROPERTY(_ClearanceSpeed);
    CK_PROPERTY(_ContactGrace);
    CK_PROPERTY(_Gravity);
    CK_PROPERTY(_QueryFilter);
};

USTRUCT(BlueprintType)
struct CKPROCEDURALANIMATION_API FCk_Request_SurfaceMotion_Steering : public FCk_Request_Base
{
    GENERATED_BODY()
    CK_GENERATED_BODY(FCk_Request_SurfaceMotion_Steering);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_SurfaceMotion_Steering);

private:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _WorldDirection = FVector::ForwardVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _Speed = 0.0f;
public:
    CK_PROPERTY(_WorldDirection);
    CK_PROPERTY(_Speed);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_SurfaceMotion_Steering, _WorldDirection, _Speed);
};
