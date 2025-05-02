#include "CkIsmRenderer_Fragment_Data.h"

#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Ism_Renderer, TEXT("Ism.Renderer"));

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_IsmRenderer_Data::
    PostEditChangeProperty(
        FPropertyChangedEvent& InPropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(InPropertyChangedEvent);

    if (InPropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UCk_IsmRenderer_Data, _Mobility))
    {
        if (_Mobility == ECk_Mobility::Static)
        {
            _UpdatePolicy = ECk_Ism_InstanceUpdatePolicy::Recreate;
        }
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------
