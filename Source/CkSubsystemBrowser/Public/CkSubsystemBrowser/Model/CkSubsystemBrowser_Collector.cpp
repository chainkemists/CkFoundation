#include "CkSubsystemBrowser/Model/CkSubsystemBrowser_Collector.h"

#include "Editor.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

#include "EditorSubsystem.h"
#include "Subsystems/EngineSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Subsystems/WorldSubsystem.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::subsystem_browser
{
    namespace
    {
        // Prefer a running PIE world; fall back to the editor world so World
        // subsystems are still browsable outside of play.
        auto
            Get_ActiveWorld()
            -> UWorld*
        {
            if (GEditor == nullptr)
            { return nullptr; }

            for (const auto& Context : GEditor->GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::PIE && Context.World() != nullptr)
                { return Context.World(); }
            }

            return GEditor->GetEditorWorldContext().World();
        }

        // GameInstance / LocalPlayer subsystems only exist while playing.
        auto
            Get_ActiveGameInstance()
            -> UGameInstance*
        {
            if (GEditor == nullptr)
            { return nullptr; }

            for (const auto& Context : GEditor->GetWorldContexts())
            {
                if (Context.WorldType == EWorldType::PIE && Context.OwningGameInstance != nullptr)
                { return Context.OwningGameInstance; }
            }

            return nullptr;
        }
    }

    auto
        Get_AllCategories()
        -> TArray<ECk_SubsystemCategory>
    {
        return {
            ECk_SubsystemCategory::Engine,
            ECk_SubsystemCategory::Editor,
            ECk_SubsystemCategory::GameInstance,
            ECk_SubsystemCategory::World,
            ECk_SubsystemCategory::LocalPlayer
        };
    }

    auto
        Get_CategoryDisplayName(
            ECk_SubsystemCategory InCategory)
        -> FString
    {
        switch (InCategory)
        {
            case ECk_SubsystemCategory::Engine:       return TEXT("Engine");
            case ECk_SubsystemCategory::Editor:       return TEXT("Editor");
            case ECk_SubsystemCategory::GameInstance: return TEXT("Game Instance");
            case ECk_SubsystemCategory::World:        return TEXT("World");
            case ECk_SubsystemCategory::LocalPlayer:  return TEXT("Local Player");
            default:                                  return TEXT("Unknown");
        }
    }

    auto
        Collect_Subsystems(
            ECk_SubsystemCategory InCategory)
        -> TArray<TWeakObjectPtr<UObject>>
    {
        auto Out = TArray<TWeakObjectPtr<UObject>>{};

        switch (InCategory)
        {
            case ECk_SubsystemCategory::Engine:
            {
                if (GEngine != nullptr)
                {
                    for (auto* Subsystem : GEngine->GetEngineSubsystemArrayCopy<UEngineSubsystem>())
                    { Out.Add(Subsystem); }
                }
                break;
            }
            case ECk_SubsystemCategory::Editor:
            {
                if (GEditor != nullptr)
                {
                    for (auto* Subsystem : GEditor->GetEditorSubsystemArrayCopy<UEditorSubsystem>())
                    { Out.Add(Subsystem); }
                }
                break;
            }
            case ECk_SubsystemCategory::World:
            {
                if (auto* World = Get_ActiveWorld())
                {
                    for (auto* Subsystem : World->GetSubsystemArrayCopy<UWorldSubsystem>())
                    { Out.Add(Subsystem); }
                }
                break;
            }
            case ECk_SubsystemCategory::GameInstance:
            {
                if (auto* GameInstance = Get_ActiveGameInstance())
                {
                    for (auto* Subsystem : GameInstance->GetSubsystemArrayCopy<UGameInstanceSubsystem>())
                    { Out.Add(Subsystem); }
                }
                break;
            }
            case ECk_SubsystemCategory::LocalPlayer:
            {
                if (auto* GameInstance = Get_ActiveGameInstance())
                {
                    for (auto* LocalPlayer : GameInstance->GetLocalPlayers())
                    {
                        if (LocalPlayer == nullptr)
                        { continue; }

                        for (auto* Subsystem : LocalPlayer->GetSubsystemArrayCopy<ULocalPlayerSubsystem>())
                        { Out.Add(Subsystem); }
                    }
                }
                break;
            }
        }

        return Out;
    }
}

// --------------------------------------------------------------------------------------------------------------------
