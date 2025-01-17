#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/World/CkEcsWorld.h"

#include <Subsystems/EngineSubsystem.h>

#include "CkEditorAssetLoader_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta= (DisplayName = "CkSubsystem_EditorAssetLoader"))
class CKRESOURCELOADEREDITOR_API UCk_EditorAssetLoader_SubSystem_UE : public UEngineSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EditorAssetLoader_SubSystem_UE);

private:
    auto Initialize(
        FSubsystemCollectionBase& Collection) -> void override;

private:
    auto
    DoOnEngineInitComplete() -> void;

public:
    auto
    Request_RefreshLoadingAssets() -> void;

private:
    ck::FEcsWorld _World;
    FCk_Handle _AssetLoaderEntity;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKRESOURCELOADEREDITOR_API UCk_Utils_EditorAssetLoader_SubSystem_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    using SubsystemType = UCk_EditorAssetLoader_SubSystem_UE;

public:
    static auto
    Request_RefreshLoadingAssets() -> void;
};

// --------------------------------------------------------------------------------------------------------------------
