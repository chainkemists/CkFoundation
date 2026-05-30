#pragma once

#include "CkCamera/Camera/Profile/CkCameraProfile.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------
// The running profile is stored DECOMPOSED — one fragment per section — on the FCk_Handle_CameraProfile entity.
// UCk_Utils_CameraProfile_UE assembles them into an FCk_CameraProfile on demand (Get_Profile). Modes blend a whole
// preset in (BlendInto); Trims mutate individual sections via the per-section accessors. There is no monolithic
// "_Current" fragment.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // ---- Always-on sections ----

    struct CKCAMERA_API FFragment_CameraProfile_Rig
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_Rig);

    private:
        FCk_CameraProfile_Rig _Rig;

    public:
        CK_PROPERTY(_Rig);
    };

    struct CKCAMERA_API FFragment_CameraProfile_Springs
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_Springs);

    private:
        FCk_CameraProfile_Springs _Springs;

    public:
        CK_PROPERTY(_Springs);
    };

    struct CKCAMERA_API FFragment_CameraProfile_Sensor
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_Sensor);

    private:
        FCk_CameraProfile_Sensor _Sensor;

    public:
        CK_PROPERTY(_Sensor);
    };

    struct CKCAMERA_API FFragment_CameraProfile_Noise
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_Noise);

    private:
        FCk_CameraProfile_Noise _Noise;

    public:
        CK_PROPERTY(_Noise);
    };

    // ---- Toggled sections (carry their own enabled flag, mirroring the FCk_CameraProfile _HasX / _UsePostProcess) ----

    struct CKCAMERA_API FFragment_CameraProfile_OrientationControl
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_OrientationControl);

    private:
        bool                                 _Enabled = true;
        FCk_CameraProfile_OrientationControl _OrientationControl;

    public:
        CK_PROPERTY(_Enabled);
        CK_PROPERTY(_OrientationControl);
    };

    struct CKCAMERA_API FFragment_CameraProfile_AutoReorient
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_AutoReorient);

    private:
        bool                           _Enabled = false;
        FCk_CameraProfile_AutoReorient _AutoReorient;

    public:
        CK_PROPERTY(_Enabled);
        CK_PROPERTY(_AutoReorient);
    };

    struct CKCAMERA_API FFragment_CameraProfile_Collision
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_Collision);

    private:
        bool                        _Enabled = true;
        FCk_CameraProfile_Collision _Collision;

    public:
        CK_PROPERTY(_Enabled);
        CK_PROPERTY(_Collision);
    };

    struct CKCAMERA_API FFragment_CameraProfile_DepthOfField
    {
        CK_GENERATED_BODY(FFragment_CameraProfile_DepthOfField);

    private:
        bool                           _Enabled = false; // == FCk_CameraProfile::_UsePostProcess
        FCk_CameraProfile_DepthOfField _DepthOfField;

    public:
        CK_PROPERTY(_Enabled);
        CK_PROPERTY(_DepthOfField);
    };
}

// --------------------------------------------------------------------------------------------------------------------
