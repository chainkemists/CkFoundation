#pragma once

#include "CkCamera/Camera/CkCameraProfile.h"

#include "CkCore/Macros/CkMacros.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Holds the running composed profile on the dedicated profile entity (FCk_Handle_CameraProfile). Reset to a
    // fresh default each frame by FProcessor_Camera_ComposeProfile, then every active modifier contributes into it
    // via UCk_Utils_CameraProfile_UE. UpdatePOV reads the resolved result back through the director's cache.
    struct CKCAMERA_API FFragment_CameraProfile_Current
    {
    public:
        CK_GENERATED_BODY(FFragment_CameraProfile_Current);

    private:
        FCk_CameraProfile _Profile;

    public:
        CK_PROPERTY(_Profile);
    };
}

// --------------------------------------------------------------------------------------------------------------------
