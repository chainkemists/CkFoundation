#include "CkGroundNav/Volume/CkGroundNavVolume_EntityScript.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"
#include "CkGroundNav/Bake/CkGroundNav_AgentProfile.h"
#include "CkGroundNav/Cook/CkGroundNav_CookedFieldIndex.h"
#include "CkGroundNav/Field/CkGroundNav_FieldTypes.h"
#include "CkGroundNav/Volume/CkGroundNavVolume_Utils.h"
#include "CkJolt/Settings/CkJolt_ProjectSettings.h"

#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        Get_PlacedVolumeParams(
            const FCk_Fragment_GroundNavVolume_ParamsData& InParams,
            const FTransform& InPlacement)
        -> FCk_Fragment_GroundNavVolume_ParamsData
    {
        auto Placed = InParams;
        Placed.Set_VolumeBounds(InParams.Get_VolumeBounds().ShiftBy(InPlacement.GetTranslation()));
        return Placed;
    }
}

// --------------------------------------------------------------------------------------------------------------------

UCk_GroundNavVolume_EntityScript::
    UCk_GroundNavVolume_EntityScript(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    _Replication = ECk_Replication::DoesNotReplicate;
    _ShowInPlaceActors = true;
}

auto
    UCk_GroundNavVolume_EntityScript::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    const auto OwnerIsValid = ck::IsValid(InHandle);
    CK_ENSURE_IF_NOT(OwnerIsValid, TEXT("GroundNavVolume EntityScript requires a valid owner."))
    { }
    if (NOT OwnerIsValid)
    { return ECk_EntityScript_ConstructionFlow::Finished; }

    auto PlacedParams = ck::groundnav::Get_PlacedVolumeParams(_Params, _SpawnTransform);
    if (NOT _SpawnLevelPackage.IsNone())
    { PlacedParams.Set_CookLevelPackage(ck::groundnav::Get_PackageLookupKey(_SpawnLevelPackage.ToString())); }

    auto FieldParams = ck::groundnav::Get_VolumeFieldParams(PlacedParams, {}, {});
    const auto VolumeId = ck::groundnav::FCk_GroundNav_VolumeId{PlacedParams.Get_StreamingVolumeId()};
    auto ParamsAreValid = NOT _SpawnTransform.ContainsNaN() && FieldParams.Get_IsValid() &&
        PlacedParams.Get_ProbeBudgetPerTick() > 0 &&
        (VolumeId.Get_IsLegacy() || VolumeId.Get_IsStreamingValid());
    auto ProfileTags = TSet<FGameplayTag>{};
    for (const auto& Variant : PlacedParams.Get_ProfileVariants())
    {
        FieldParams._Profile = Variant.Get_Profile();
        const auto ProfileTag = Variant.Get_ProfileTag();
        ParamsAreValid = ParamsAreValid && ProfileTag.IsValid() &&
            NOT ProfileTags.Contains(ProfileTag) && FieldParams.Get_IsValid() &&
            ck::groundnav::Get_ProfileRejection(Variant.Get_Profile()) == ck::groundnav::EProfileRejection::None;
        ProfileTags.Add(ProfileTag);
    }

    CK_ENSURE_IF_NOT(ParamsAreValid,
        TEXT("GroundNavVolume EntityScript requires valid bounds, bake settings, volume identity and unique profiles."))
    { }
    if (NOT ParamsAreValid)
    {
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        return ECk_EntityScript_ConstructionFlow::Finished;
    }

    const auto Ret = Super::Construct(InHandle, InSpawnParams);
    const auto Transform = UCk_Utils_Transform_UE::Add(InHandle, _SpawnTransform, ECk_Replication::DoesNotReplicate);
    const auto TransformIsValid = ck::IsValid(Transform);
    CK_ENSURE_IF_NOT(TransformIsValid, TEXT("GroundNavVolume EntityScript failed to compose its transform."))
    { }
    if (NOT TransformIsValid)
    {
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        return ECk_EntityScript_ConstructionFlow::Finished;
    }

    // Editor preview baking uses the same opt-in as the Jolt geometry world.
    const auto* World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
    const auto IsEditorPreviewWorld = ck::IsValid(World) && World->WorldType == EWorldType::Editor;
    if (IsEditorPreviewWorld && UCk_Utils_Jolt_ProjectSettings::Get_EditorStaticWorldMode() ==
        ECk_Jolt_EditorStaticWorldMode::Disabled)
    { PlacedParams.Set_AutoBuildOnSetup(ECk_EnableDisable::Disable); }

    const auto Volume = UCk_Utils_GroundNavVolume_UE::Add(InHandle, PlacedParams);
    const auto IsVolumeValid = ck::IsValid(Volume);
    CK_ENSURE_IF_NOT(IsVolumeValid, TEXT("GroundNavVolume EntityScript failed to compose its volume feature."))
    { }
    if (NOT IsVolumeValid)
    {
        UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        return ECk_EntityScript_ConstructionFlow::Finished;
    }

    return Ret;
}
