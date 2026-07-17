#include "CkEditorSelectionOwner_HostActor.h"

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    ACk_EditorSelectionProxyHost_Actor_UE::
    IsSelectionChild() const
    -> bool
{
    return _EditorSelectionOwner.IsValid();
}

auto
    ACk_EditorSelectionProxyHost_Actor_UE::
    GetSelectionParent() const
    -> AActor*
{
    return _EditorSelectionOwner.Get();
}
#endif

// --------------------------------------------------------------------------------------------------------------------
