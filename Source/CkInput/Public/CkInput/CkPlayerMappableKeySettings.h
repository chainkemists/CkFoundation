#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameplayTagContainer.h>
#include <PlayerMappableKeySettings.h>

#include "CkPlayerMappableKeySettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Player-mappable key settings carrying SEMANTIC scope tags beside the engine's presentation
 * fields. DisplayCategory stays what it is for — section headers, localized text — while scope
 * tags answer "which input contexts can genuinely collide": conflict detection under
 * ECk_KeyConflictScope::SameScopeTags flags only mappings whose scope tags intersect
 * (Input.Scope.Gameplay vs Input.Scope.Station.Roulette never conflict; hierarchy queries work).
 *
 * Author on the Input Action's PlayerMappableKeySettings (or per-mapping via
 * MakeMappingPlayerMappable, which instantiates this class). Untagged mappings never match a
 * SameScopeTags scan — tag every mappable a game shows on its rebind screen.
 */
UCLASS(BlueprintType, EditInlineNew)
class CKINPUT_API UCk_PlayerMappableKeySettings_UE : public UPlayerMappableKeySettings
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_PlayerMappableKeySettings_UE);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ScopeTags;

public:
    CK_PROPERTY(_ScopeTags);
};

// --------------------------------------------------------------------------------------------------------------------
