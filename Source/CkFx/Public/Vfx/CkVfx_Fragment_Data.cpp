#include "CkVfx_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Vfx, TEXT("Vfx"));

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Vfx_PlayAttached::
    FCk_Request_Vfx_PlayAttached(
        USceneComponent* InAttachComponent)
    : _AttachComponent(InAttachComponent)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Vfx_PlayAtLocation::
    FCk_Request_Vfx_PlayAtLocation(
        UObject* InOuter,
        FCk_Vfx_TransformSettings InTransformSettings)
    : _Outer(InOuter)
    , _TransformSettings(InTransformSettings)
{
}

// --------------------------------------------------------------------------------------------------------------------
