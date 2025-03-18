#include "CkSfx_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Sfx, TEXT("Sfx"));

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Sfx_PlayAttached::
    FCk_Request_Sfx_PlayAttached(
        USceneComponent* InAttachComponent)
    : _AttachComponent(InAttachComponent)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Sfx_PlayAtLocation::
    FCk_Request_Sfx_PlayAtLocation(
        UObject* InOuter,
        FCk_Sfx_TransformSettings InTransformSettings)
    :  _Outer(InOuter)
    , _TransformSettings(InTransformSettings)
{
}

// --------------------------------------------------------------------------------------------------------------------

