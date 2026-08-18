#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkSnapshot_PostureRatchet_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Project-supplied inputs to the fragment-posture coverage fence. The fence itself is generic and lives with the
// tests; the DEBT it guards is per-project, so it is config a project fills in once and then drains to nothing.
UCLASS(meta = (DisplayName = "Snapshot Posture Ratchet"))
class CKSNAPSHOT_API UCk_Snapshot_PostureRatchet_Settings : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Snapshot_PostureRatchet_Settings);

public:
    UCk_Snapshot_PostureRatchet_Settings(
        const FObjectInitializer& InObjectInitializer);

private:
    // Reflected-name prefixes (UHT strips the leading F, so FBb_Fragment_X reflects as Bb_Fragment_X) of the
    // script-declared fragment types the fence enumerates. There is no reflected marker separating a dynamic
    // fragment from any other script struct, so this list defines COVERAGE: a fragment named outside it is
    // invisible. That makes it as load-bearing as the allow-list, and it is ratcheted the same way.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Posture Ratchet",
              meta = (AllowPrivateAccess = true))
    TArray<FString> _FragmentNamePrefixes;

    // Reflected names of enumerated types that still resolve Undeclared. Every entry is debt with one of two
    // dispositions: declare a posture, or split the fragment. A type that resolves Undeclared and is NOT listed
    // here fails the fence, so a fragment authored today is red today.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Posture Ratchet",
              meta = (AllowPrivateAccess = true))
    TArray<FName> _UndeclaredFragmentAllowList;

    // The project's declared ceiling on the list above. It may only ever DECREASE: the fence refuses a ceiling
    // above the commissioned one, so paid-down debt cannot silently grow back.
    UPROPERTY(Config, EditDefaultsOnly, Category = "Posture Ratchet",
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _AllowListCeiling = 0;

public:
    CK_PROPERTY_GET(_FragmentNamePrefixes);
    CK_PROPERTY_GET(_UndeclaredFragmentAllowList);
    CK_PROPERTY_GET(_AllowListCeiling);
};

// --------------------------------------------------------------------------------------------------------------------
