#pragma once

#include "CkCore/Enums/CkEnums.h"

#include "CkPathNetwork/Network/CkPathNetwork_Fragment_Data.h"
#include "CkPathNetwork/Network/CkPathNetwork_Types.h"

#include <CoreMinimal.h>
#include <UObject/WeakObjectPtr.h>

class ACk_PathNetwork_UE;
class UCk_PathNetwork_Detector_UE;
class ULevel;
class UWorld;

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pathnetwork_editor::authoring
{
    // Detector callbacks used by Preview are a read-only extension point. Implementations may
    // inspect the supplied editor world but must not mutate objects, packages, selection, actors,
    // or transactions. The service checks common invariants and rejects detected breaches.
    struct CKPATHNETWORKEDITOR_API FPreviewRequest
    {
        UWorld* _World = nullptr;
        const UCk_PathNetwork_Detector_UE* _DetectorTemplate = nullptr;
        const ACk_PathNetwork_UE* _SourceActor = nullptr;
        FBox _DetectionBounds = FBox{ForceInit};
        FCk_PathNetwork_VectorizeParams _VectorizeParams;
    };

    struct CKPATHNETWORKEDITOR_API FPreviewResult
    {
        bool _Succeeded = false;
        FString _FailureReason;
        TWeakObjectPtr<UWorld> _World;
        FBox _DetectionBounds = FBox{ForceInit};
        FCk_PathNetwork_VectorizeParams _VectorizeParams;
        TWeakObjectPtr<UClass> _DetectorClass;
        uint32 _DetectorConfigurationFingerprint = 0;
        FCk_PathNetwork_DetectionMask _Mask;
        TArray<FCk_PathNetwork_Ribbon> _GeneratedWorldRibbons;
        int32 _OccupiedCellCount = 0;
        int32 _UnsupportedSegmentCount = 0;
    };

    struct CKPATHNETWORKEDITOR_API FApplyToLevelRequest
    {
        ULevel* _TargetLevel = nullptr;
        ACk_PathNetwork_UE* _ExplicitTargetActor = nullptr;
        const UCk_PathNetwork_Detector_UE* _DetectorTemplate = nullptr;
        FBox _DetectionBounds = FBox{ForceInit};
        FCk_PathNetwork_VectorizeParams _VectorizeParams;
        FCk_PathNetwork_BuildParams _BuildParams;
        ECk_EnableDisable _AutoDetectOnBeginPlay = ECk_EnableDisable::Disable;
        ECk_EnableDisable _UseRecommendedFollowerTuning = ECk_EnableDisable::Disable;
        FCk_PathNetworkFollower_Tuning _RecommendedFollowerTuning;
        const FPreviewResult* _Preview = nullptr;
    };

    struct CKPATHNETWORKEDITOR_API FApplyResult
    {
        bool _Succeeded = false;
        bool _CreatedActor = false;
        FString _FailureReason;
        TWeakObjectPtr<ACk_PathNetwork_UE> _Actor;
        int32 _AuthoredRibbonCount = 0;
        int32 _GeneratedRibbonCount = 0;
        int32 _TotalRibbonCount = 0;
    };

    CKPATHNETWORKEDITOR_API auto
    Is_UsableDetectorClass(
        const UClass* InClass) -> bool;

    CKPATHNETWORKEDITOR_API auto
    Get_LoadedUsableDetectorClasses() -> TArray<UClass*>;

    CKPATHNETWORKEDITOR_API auto
    Compute_DetectorConfigurationFingerprint(
        const UCk_PathNetwork_Detector_UE* InDetector) -> uint32;

    CKPATHNETWORKEDITOR_API auto
    Preview(
        const FPreviewRequest& InRequest) -> FPreviewResult;

    // Legacy actor bake path. It mutates ribbons only and deliberately preserves detector object
    // identity and all actor authoring configuration.
    CKPATHNETWORKEDITOR_API auto
    ApplyPreview_ToExistingActor(
        ACk_PathNetwork_UE* InActor,
        const FPreviewResult& InPreview) -> FApplyResult;

    CKPATHNETWORKEDITOR_API auto
    ApplyPreview_ToLevel(
        const FApplyToLevelRequest& InRequest) -> FApplyResult;
}

// --------------------------------------------------------------------------------------------------------------------
