#pragma once

#include "CkCore/Subsystems/GameWorldSubsytem/CkGameWorldSubsystem.h"
#include "CkRenderProxy/Manager/CkRenderProxyManager_Actor.h"

#include "CkRenderProxyManager_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_RenderProxyManager")
class CKRENDERPROXY_API UCk_RenderProxyManager_Subsystem : public UCk_Game_WorldSubsystem_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_RenderProxyManager_Subsystem);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

public:
    auto Get_ManagerActor() const -> ACk_RenderProxyManager_Actor*;
    auto Get_ManagerComponent() const -> UCk_Component_RenderProxyManager*;

private:
    UPROPERTY(Transient)
    TObjectPtr<ACk_RenderProxyManager_Actor> _ManagerActor;
};

// --------------------------------------------------------------------------------------------------------------------