#include "CkPathNetworkEditor/Designer/CkPathNetwork_DesignerPreset.h"

#include "CkPathNetworkEditor/Authoring/CkPathNetwork_AuthoringService.h"

#include "CkPathNetwork/Network/CkPathNetwork_Utils.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer::preset_detail
{
    TArray<FPreset> GPresets;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer
{
    auto
    Register_Preset(
        const FPreset& InPreset) -> bool
    {
        const bool IdentityIsValid =
            NOT InPreset._Owner.IsNone()
            && NOT InPreset._Id.IsNone()
            && NOT InPreset._DisplayName.IsEmpty();
        CK_ENSURE_IF_NOT(IdentityIsValid,
            TEXT("Path-network designer preset requires owner, id, and display name"))
        {}
        if (NOT IdentityIsValid)
        { return false; }

        const bool DetectorClassIsValid =
            authoring::Is_UsableDetectorClass(InPreset._DetectorClass.Get());
        CK_ENSURE_IF_NOT(DetectorClassIsValid,
            TEXT("Path-network designer preset [{}] requires a usable detector class"),
            InPreset._Id)
        {}
        if (NOT DetectorClassIsValid)
        { return false; }

        const bool RecommendationIsValid =
            InPreset._UseRecommendedFollowerTuning != ECk_EnableDisable::Enable
            || UCk_Utils_PathNetworkFollower_UE::Get_IsTuningValid(
                InPreset._RecommendedFollowerTuning);
        CK_ENSURE_IF_NOT(RecommendationIsValid,
            TEXT("Path-network designer preset [{}] contains invalid route preferences"),
            InPreset._Id)
        {}
        if (NOT RecommendationIsValid)
        { return false; }

        const bool IsUnique = NOT preset_detail::GPresets.ContainsByPredicate(
            [&](const FPreset& InExisting)
            {
                return InExisting._Owner == InPreset._Owner
                    && InExisting._Id == InPreset._Id;
            });
        CK_ENSURE_IF_NOT(IsUnique,
            TEXT("Path-network designer preset [{}:{}] is already registered"),
            InPreset._Owner, InPreset._Id)
        {}
        if (NOT IsUnique)
        { return false; }

        preset_detail::GPresets.Add(InPreset);
        return true;
    }

    auto
    Unregister_PresetsByOwner(
        const FName InOwner) -> void
    {
        preset_detail::GPresets.RemoveAll(
            [&](const FPreset& InPreset)
            {
                return InPreset._Owner == InOwner;
            });
    }

    auto
    Get_Presets() -> TArray<FPreset>
    {
        auto Result = preset_detail::GPresets;
        Result.Sort([](const FPreset& InLhs, const FPreset& InRhs)
        {
            if (InLhs._SortPriority != InRhs._SortPriority)
            { return InLhs._SortPriority > InRhs._SortPriority; }
            return InLhs._DisplayName.ToString() < InRhs._DisplayName.ToString();
        });
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
