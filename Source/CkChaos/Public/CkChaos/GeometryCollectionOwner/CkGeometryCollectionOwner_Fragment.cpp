#include "CkGeometryCollectionOwner_Fragment.h"

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment.h"
#include "CkChaos/GeometryCollection/CkGeometryCollection_Utils.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include "TargetPoint/CkTargetPoint_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    Broadcast_ApplyStrain(
        const FCk_Request_GeometryCollectionOwner_ApplyStrain_Replicated& InApplyStrain)
    -> void
{
    _Strain.Emplace(InApplyStrain);
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_GeometryCollectionOwner_Rep, _Strain, this);
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    Broadcast_ApplyAoE(
        const FCk_Request_GeometryCollectionOwner_ApplyAoE_Replicated& InAoE)
    -> void
{
    _AoE.Emplace(InAoE);
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_GeometryCollectionOwner_Rep, _AoE, this);
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    Broadcast_CrumbleNonActiveClusters()
    -> void
{
    ++_CrumbleNonActiveClustersRequest;
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_GeometryCollectionOwner_Rep, _CrumbleNonActiveClustersRequest, this);
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    Broadcast_RemoveAllAnchors()
    -> void
{
    ++_RemoveAllAnchors;
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_GeometryCollectionOwner_Rep, _RemoveAllAnchors, this);
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    Broadcast_RemoveAllAnchorsAndCrumbleNonActiveClusters()
    -> void
{
    ++_RemoveAllAnchorsAndCrumbleNonActiveClusters;
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_GeometryCollectionOwner_Rep, _RemoveAllAnchorsAndCrumbleNonActiveClusters, this);
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _CrumbleNonActiveClustersRequest, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _RemoveAllAnchors, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _RemoveAllAnchorsAndCrumbleNonActiveClusters, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Strain, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _AoE, Params);
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    PostLink()
    -> void
{
    Super::PostLink();

    OnRep_Updated();
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    OnRep_Updated()
    -> void
{
    auto Entity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(Entity))
    { return; }

    for (; _Strain_LastValidIndex < _Strain.Num(); ++_Strain_LastValidIndex)
    {
        ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
        {
            const auto& Request = _Strain[_Strain_LastValidIndex];
            UCk_Utils_GeometryCollection_UE::Request_ApplyStrainAndVelocity(InGc, FCk_Request_GeometryCollection_ApplyStrain
                {
                    Request.Get_Location(),
                    Request.Get_Request()->Get_Radius()
                }
                .Set_InternalStrain(Request.Get_Request()->Get_InternalStrain())
                .Set_ExternalStrain(Request.Get_Request()->Get_ExternalStrain())
                .Set_LinearVelocity(Request.Get_Request()->Get_LinearVelocity())
                .Set_AngularVelocity(Request.Get_Request()->Get_AngularVelocity())
            );
        });
    }

    for (; _AoE_LastValidIndex < _AoE.Num(); ++_AoE_LastValidIndex)
    {
        ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
        {
            const auto& Request = _AoE[_Strain_LastValidIndex];
            UCk_Utils_GeometryCollection_UE::Request_ApplyAoE(InGc, FCk_Request_GeometryCollection_ApplyAoE
                {
                    Request.Get_Location(),
                    Request.Get_Request()->Get_Radius()
                }
                .Set_InternalStrain(Request.Get_Request()->Get_InternalStrain())
                .Set_ExternalStrain(Request.Get_Request()->Get_ExternalStrain())
                .Set_LinearSpeed(Request.Get_Request()->Get_LinearSpeed())
                .Set_AngularSpeed(Request.Get_Request()->Get_AngularSpeed())
            );
        });
    }

}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    OnRep_CrumbleNonActiveClustersRequest()
    -> void
{
    auto Entity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(Entity))
    { return; }

    ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
    {
        UCk_Utils_GeometryCollection_UE::Request_CrumbleNonAnchoredClusters(InGc);
    });
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    OnRep_RemoveAllAnchors()
    -> void
{
    auto Entity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(Entity))
    { return; }

    ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
    {
        UCk_Utils_GeometryCollection_UE::Request_RemoveAllAnchors(InGc);
    });
}

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    OnRep_CrumbleNonActiveClustersAndRemoveAllAnchors()
    -> void
{
    auto Entity = Get_AssociatedEntity();

    if (ck::Is_NOT_Valid(Entity))
    { return; }

    ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
    {
        UCk_Utils_GeometryCollection_UE::Request_RemoveAllAnchors(InGc);
        UCk_Utils_GeometryCollection_UE::Request_CrumbleNonAnchoredClusters(InGc);
    });
}
