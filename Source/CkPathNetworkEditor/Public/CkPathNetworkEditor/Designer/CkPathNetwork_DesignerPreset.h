#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkPathNetwork/Detector/CkPathNetwork_Detector.h"
#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"
#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::designer
{
    // Code-registered editor preset. A game supplies only a detector class and generic authoring
    // defaults; CkPathNetworkEditor remains unaware of the game module.
    struct CKPATHNETWORKEDITOR_API FPreset
    {
        FName _Owner;
        FName _Id;
        FText _DisplayName;
        FText _Description;
        TSubclassOf<UCk_PathNetwork_Detector_UE> _DetectorClass;
        FVector _DetectionExtents = FVector{50000.0, 50000.0, 10000.0};
        FCk_PathNetwork_VectorizeParams _VectorizeParams;
        FCk_PathNetwork_BuildParams _BuildParams;
        ECk_EnableDisable _AutoDetectOnBeginPlay = ECk_EnableDisable::Disable;
        ECk_EnableDisable _UseRecommendedFollowerTuning = ECk_EnableDisable::Disable;
        FCk_PathNetworkFollower_Tuning _RecommendedFollowerTuning;
        int32 _SortPriority = 0;
    };

    CKPATHNETWORKEDITOR_API auto
    Register_Preset(
        const FPreset& InPreset) -> bool;

    CKPATHNETWORKEDITOR_API auto
    Unregister_PresetsByOwner(
        FName InOwner) -> void;

    CKPATHNETWORKEDITOR_API auto
    Get_Presets() -> TArray<FPreset>;
}

// --------------------------------------------------------------------------------------------------------------------
