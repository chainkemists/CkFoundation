#include "CkIsmRenderer_Fragment_Data.h"

#include "CkCore/Validation/CkIsValid.h"

#include <Engine/StaticMesh.h>
#include <NativeGameplayTags.h>

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Label_Ism_Renderer, TEXT("Ism.Renderer"));

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_Data::
    Get_Mesh() const
    -> const TObjectPtr<UStaticMesh>&
{
    // _Mesh wins when populated. Otherwise lazily resolve the soft ref and cache it forward so
    // later reads are free and order-independent across the various consumption sites. Safe here
    // because every caller runs post-init (Setup/GetOrCreate/proxy), where blocking loads are OK —
    // unlike the AS-load-time path that assets::load::Foo() used to take, which ensured during cook.
    if (ck::Is_NOT_Valid(_Mesh) && _MeshSoft.IsNull() == false)
    {
        const_cast<UCk_IsmRenderer_Data*>(this)->_Mesh = _MeshSoft.LoadSynchronous();
    }

    return _Mesh;
}

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
