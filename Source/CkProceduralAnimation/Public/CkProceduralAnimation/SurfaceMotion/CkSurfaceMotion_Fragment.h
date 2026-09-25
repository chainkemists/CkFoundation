#pragma once

#include "CkProceduralAnimation/SurfaceMotion/CkSurfaceMotion_Fragment_Data.h"

class UCk_Utils_SurfaceMotion_UE;
class UCk_Utils_ProceduralAnimation_Debug_UE;

namespace ck
{
    using FFragment_SurfaceMotion_Params = FCk_Fragment_SurfaceMotion_ParamsData;

    struct CKPROCEDURALANIMATION_API FFragment_SurfaceMotion_Current
    {
        CK_GENERATED_BODY(FFragment_SurfaceMotion_Current);
        friend class FProcessor_SurfaceMotion_Update;
        friend class FProcessor_SurfaceMotion_HandleRequests;
        friend class UCk_Utils_SurfaceMotion_UE;
        friend class FProcessor_ProceduralGait_Update;
        friend class UCk_Utils_ProceduralAnimation_Debug_UE;
    private:
        FVector _Direction = FVector::ForwardVector;
        FVector _Velocity = FVector::ZeroVector;
        FVector _SupportNormal = FVector::UpVector;
        FVector _TravelTangent = FVector::ForwardVector;
        float _Speed = 0.0f;
        uint64 _DebugFrameNumber = 0;
        FCk_Time _MissingContact = FCk_Time::ZeroSecond();
        bool _Grounded = false;
        bool _TrustedContact = false;
        bool _Ready = false;
    };

    struct CKPROCEDURALANIMATION_API FFragment_SurfaceMotion_Requests
    {
        CK_GENERATED_BODY(FFragment_SurfaceMotion_Requests);
        friend class FProcessor_SurfaceMotion_HandleRequests;
        friend class UCk_Utils_SurfaceMotion_UE;
    private:
        TArray<FCk_Request_SurfaceMotion_Steering> _Requests;
    public:
        CK_PROPERTY_GET(_Requests);
    };
}
