#pragma once

#include <GameFramework/Actor.h>

#include "CkEditorSelectionOwner_HostActor.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Editor-world host for preview components created on behalf of a placed actor: a viewport click on any
// hosted primitive redirects selection to the owner (see ck::FFragment_EditorSelectionOwner).
UCLASS(NotBlueprintable, NotPlaceable)
class CKECS_API ACk_EditorSelectionProxyHost_Actor_UE : public AActor
{
    GENERATED_BODY()

public:
    friend class UCk_EditorEcsWorld_Subsystem_UE;

#if WITH_EDITOR
public:
    auto
    IsSelectionChild() const -> bool override;

    auto
    GetSelectionParent() const -> AActor* override;
#endif

#if WITH_EDITORONLY_DATA
private:
    // Non-UPROPERTY on purpose: this actor is transient and must never round-trip through the
    // transaction buffer; the owner is a level actor whose lifetime the weak ptr observes.
    TWeakObjectPtr<AActor> _EditorSelectionOwner;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
