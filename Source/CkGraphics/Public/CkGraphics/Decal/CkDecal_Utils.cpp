#include "CkDecal_Utils.h"

#include "CkCore/Actor/CkActor_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkGraphics/Decal/CkDecal_Fragment.h"

#include "CkVariables/CkUnrealVariables_Utils.h"

#include <Components/DecalComponent.h>

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_HAS_CAST_CONV_HANDLE_TYPESAFE(Decal, UCk_Utils_Decal_UE, FCk_Handle_Decal, ck::FFragment_Decal_Params, ck::FFragment_Decal_Current)

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Decal_UE::
    Add(
        FCk_Handle& InHandle,
        UDecalComponent* InDecalComponent,
        const FCk_Fragment_Decal_ParamsData& InParams)
    -> FCk_Handle_Decal
{
    InHandle.Add<ck::FFragment_Decal_Params>(InParams);
    InHandle.Add<ck::FFragment_Decal_Current>(InDecalComponent);

    return Cast(InHandle);
}

auto
    UCk_Utils_Decal_UE::
    Create_FromActor(
        const FCk_Handle& InAnyValidHandle,
        TSubclassOf<ADecalActor> InDecalActorClass,
        const FTransform& InTransform)
    -> FCk_Handle_Decal
{
    // TODO: Make this deferred

    auto DecalEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAnyValidHandle);

    const auto& SpawnedActor = UCk_Utils_Actor_UE::Request_SpawnActor
    (
        FCk_Utils_Actor_SpawnActor_Params
        {
            UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyValidHandle),
            InDecalActorClass
        }
        .Set_SpawnTransform(InTransform)
        .Set_CollisionHandlingOverride(ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
        .Set_NetworkingType(ECk_Actor_NetworkingType::Local)
    );

    const auto& SpawnedDecalActor = ::Cast<ADecalActor>(SpawnedActor);

    CK_ENSURE_IF_NOT(ck::IsValid(SpawnedDecalActor), TEXT("Failed to Spawn Decal Actor"))
    { return {}; }

    const auto& DecalComponent = SpawnedDecalActor->GetDecal();

    CK_ENSURE_IF_NOT(ck::IsValid(DecalComponent), TEXT("Decal Actor [{}] has an INVALID Decal Component"), SpawnedDecalActor)
    { return {}; }

    const auto DecalLifetime = DecalComponent->bDestroyOwnerAfterFade ? ECk_Decal_Lifetime::DestroyAfterFadeOut : ECk_Decal_Lifetime::Persistent;
    const auto DecalVisualInfo = FCk_Decal_VisualInfo{DecalComponent->GetDecalMaterial()}
                                    .SetSortOrder(DecalComponent->SortOrder)
                                    .Set_DecalColor(DecalComponent->DecalColor)
                                    .Set_DecalSize(DecalComponent->DecalSize);
    const auto DecalFadeInfo = FCk_Decal_FadeInfo{}
                                .Set_HasFadeOut(DecalComponent->FadeDuration > 0.0f || DecalComponent->FadeStartDelay > 0.0f)
                                .Set_FadeOutStartDelay(FCk_Time{DecalComponent->FadeStartDelay})
                                .Set_FadeOutDuration(FCk_Time{DecalComponent->FadeDuration})
                                .Set_FadeInStartDelay(FCk_Time{DecalComponent->FadeInStartDelay})
                                .Set_FadeInDuration(FCk_Time{DecalComponent->FadeInDuration});

    Add
    (
        DecalEntity,
        DecalComponent,
        FCk_Fragment_Decal_ParamsData{}
            .Set_DecalLifetime(DecalLifetime)
            .Set_DecalFadeInfo(DecalFadeInfo)
            .Set_DecalVisualInfo(DecalVisualInfo)
    );

    return Cast(DecalEntity);
}

auto
    UCk_Utils_Decal_UE::
    Create(
        const FCk_Handle& InAnyValidHandle,
        const FTransform& InTransform,
        const FCk_Fragment_Decal_ParamsData& InDecalParams)
    -> FCk_Handle_Decal
{
    auto DecalEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(InAnyValidHandle);

    UCk_Utils_Actor_UE::Request_SpawnActor
    (
        FCk_Utils_Actor_SpawnActor_Params
        {
            UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InAnyValidHandle),
            ADecalActor::StaticClass()
        }
        .Set_SpawnTransform(InTransform)
        .Set_CollisionHandlingOverride(ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
        .Set_SpawnPolicy(ECk_Utils_Actor_SpawnActorPolicy::CannotSpawnInPersistentLevel)
        .Set_NetworkingType(ECk_Actor_NetworkingType::Local),
        [&](AActor* InActor)
        {
            const auto& DecalActor = ::Cast<ADecalActor>(InActor);

            if (ck::Is_NOT_Valid(DecalActor))
            { return; }

             auto* DecalComponent = DecalActor->GetDecal();

            if (ck::Is_NOT_Valid(DecalComponent))
            { return; }

            const auto& FadeInfo = InDecalParams.Get_DecalFadeInfo();
            const auto& VisualInfo = InDecalParams.Get_DecalVisualInfo();
            const auto& DestroyOwnerAfterFade = InDecalParams.Get_DecalLifetime() == ECk_Decal_Lifetime::DestroyAfterFadeOut;

            DecalComponent->SetDecalMaterial(VisualInfo.Get_DecalMaterial());
            DecalComponent->SetDecalColor(VisualInfo.Get_DecalColor());
            DecalComponent->SetSortOrder(VisualInfo.GetSortOrder());
            DecalComponent->DecalSize = VisualInfo.Get_DecalSize();

            DecalComponent->bDestroyOwnerAfterFade = DestroyOwnerAfterFade;

            if (FadeInfo.Get_HasFadeOut())
            {
                DecalComponent->SetFadeOut
                (
                    FadeInfo.Get_FadeOutStartDelay().Get_Seconds(),
                    FadeInfo.Get_FadeOutDuration().Get_Seconds(),
                    DestroyOwnerAfterFade
                );
            }

            DecalComponent->SetFadeIn(FadeInfo.Get_FadeInStartDelay().Get_Seconds(), FadeInfo.Get_FadeInDuration().Get_Seconds());

            Add(DecalEntity, DecalComponent, InDecalParams);
        }
    );

    return Cast(DecalEntity);
}

// --------------------------------------------------------------------------------------------------------------------
