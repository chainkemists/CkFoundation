#include "CkCamera_Fragment.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

// Root for camera-modifier ordering-group tags. Constrains the FCk_Request_Camera_AddModifier::_OrderingGroup picker
// (meta = (Categories = "Camera.OrderingGroup")). Projects define children under it for their own stacking layers.
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Camera_OrderingGroup, TEXT("Camera.OrderingGroup"));

// --------------------------------------------------------------------------------------------------------------------
