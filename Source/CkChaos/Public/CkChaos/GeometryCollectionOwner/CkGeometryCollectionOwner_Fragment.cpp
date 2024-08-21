#include "CkGeometryCollectionOwner_Fragment.h"

#include "CkChaos/GeometryCollection/CkGeometryCollection_Fragment.h"
#include "CkChaos/GeometryCollection/CkGeometryCollection_Utils.h"

#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include "TargetPoint/CkTargetPoint_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_GeometryCollectionOwner_Rep::
    Broadcast_ApplyRadianStrain(
        const FCk_Request_GeometryCollectionOwner_ApplyRadialStrain_Replicated& InRadialStrain)
    -> void
{
    _RadialStrains.Emplace(InRadialStrain);
    MARK_PROPERTY_DIRTY_FROM_NAME(UCk_Fragment_GeometryCollectionOwner_Rep, _RadialStrains, this);
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
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _RadialStrains, Params);
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

    for (; _RadialStrains_LastValidIndex < _RadialStrains.Num(); ++_RadialStrains_LastValidIndex)
    {
        ck::FUtils_RecordOfGeometryCollections::ForEach_ValidEntry(Entity, [&](FCk_Handle_GeometryCollection InGc)
        {
            const auto& Request = _RadialStrains[_RadialStrains_LastValidIndex];
            UCk_Utils_GeometryCollection_UE::Request_ApplyRadialStrain(InGc, FCk_Request_GeometryCollection_ApplyRadialStrain
                {
                    Request.Get_Location(),
                    Request.Get_Request()->Get_Radius()
                }
                .Set_InternalStrain(Request.Get_Request()->Get_InternalStrain())
                .Set_ExternalStrain(Request.Get_Request()->Get_ExternalStrain())
                .Set_LinearSpeed(Request.Get_Request()->Get_LinearSpeed())
                .Set_AngularSpeed(Request.Get_Request()->Get_AngularSpeed())
                .Set_ChangeParticleStateTo(Request.Get_Request()->Get_ChangeParticleStateTo())
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
