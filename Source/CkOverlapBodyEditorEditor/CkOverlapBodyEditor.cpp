#include "CkOverlapBodyEditor.h"

#include "CkLog/CkLog_Utils.h"

#include "CkOverlapBodyEditor_Log.h"

#include "CkCore/Public/CkCore/Macros/CkMacros.h"

#include <Editor/EditorEngine.h>

#define LOCTEXT_NAMESPACE "FCkOverlapBodyEditorModule"

auto
    FCkOverlapBodyEditorModule::
    StartupModule()
    -> void
{
    const auto SetupCollisionProfiles = [this](UCollisionProfile* InCollisionProfile, float InWaitTime)
    {
        if ( UEditorEngine* EditorEngine = Cast<UEditorEngine>(GEngine))
        {
            if (_TryReloading)
            {
                _TryReloading = false;
                const auto& TimerManager = EditorEngine->GetTimerManager();

                if (_CallbackTimer.IsValid())
                { return; }

                TimerManager->SetTimer(_CallbackTimer, [InCollisionProfile]()
                    {
                        InCollisionProfile->LoadProfileConfig();
                    }, InWaitTime, false);

                return;
            }

            _CallbackTimer.Invalidate();
        }

        auto CollisionProfile = InCollisionProfile;

        auto SensorChannelName = FName{TEXT("CkSensor")};
        auto MarkerChannelName = FName{TEXT("CkMarker")};

        uint8 SensorChannelIndex = INDEX_NONE;
        uint8 MarkerChannelIndex = INDEX_NONE;

        // these magic numbers are coming from EngineTypes.h:1331
        constexpr auto ChannelStartingIndex = 14;
        constexpr auto ChannelMaxIndex = 32;

        for (auto Index =  ChannelStartingIndex; Index < ChannelMaxIndex; Index++)
        {
            const auto ChannelName = CollisionProfile->ReturnChannelNameFromContainerIndex(Index);
            if (ChannelName == SensorChannelName)
            {
                SensorChannelIndex = Index;
            }
            else if (ChannelName == MarkerChannelName)
            {
                MarkerChannelIndex = Index;
            }
        }

        CK_LOG_ERROR_NOTIFY_IF_NOT(ck::overlap_body_editor, SensorChannelIndex < ChannelMaxIndex,
            TEXT("Expected to find object channel [{}]. Please add it to your Project Settings > Collision"),
            SensorChannelName)
        {
            _TryReloading = true;
            return;
        }

        CK_LOG_ERROR_NOTIFY_IF_NOT(ck::overlap_body_editor, MarkerChannelIndex < ChannelMaxIndex,
            TEXT("Expected to find object channel [{}]. Please add it to your Project Settings > Collision"),
            MarkerChannelName)
        {
            _TryReloading = true;
            return;
        }

        {
            FCollisionResponseTemplate Profile;
            const auto ObjectType = TEnumAsByte<ECollisionChannel>{SensorChannelIndex};
            if (NOT InCollisionProfile->GetProfileTemplate(SensorChannelName, Profile))
            {
                Profile.Name = SensorChannelName;
                Profile.CollisionEnabled = ECollisionEnabled::QueryOnly;
                Profile.ObjectType = ObjectType;
                Profile.ObjectTypeName = SensorChannelName;
                Profile.bCanModify = false;
                Profile.ResponseToChannels = FCollisionResponseContainer::GetDefaultResponseContainer();
                Profile.ResponseToChannels.Visibility = ECR_Ignore;
                Profile.ResponseToChannels.Camera = ECR_Ignore;
                Profile.ResponseToChannels.WorldStatic = ECR_Ignore;
                Profile.ResponseToChannels.WorldDynamic = ECR_Ignore;
                Profile.ResponseToChannels.Pawn = ECR_Ignore;
                Profile.ResponseToChannels.PhysicsBody = ECR_Ignore;
                Profile.ResponseToChannels.Vehicle = ECR_Ignore;
                Profile.ResponseToChannels.Destructible = ECR_Ignore;
                Profile.ResponseToChannels.EnumArray[SensorChannelIndex] = ECR_Ignore;
                Profile.ResponseToChannels.EnumArray[MarkerChannelIndex] = ECR_Overlap;
#if WITH_EDITORONLY_DATA
                Profile.HelpMessage = TEXT("Sensor detects Markers");
#endif
            }

            FCollisionProfilePrivateAccessor::AddProfileTemplate(Profile);
        }

        {
            const auto ProfileName = TEXT("CkSensor_Static");
            const auto ObjectType = TEnumAsByte<ECollisionChannel>{SensorChannelIndex};
            FCollisionResponseTemplate Profile;
            if (NOT InCollisionProfile->GetProfileTemplate(ProfileName, Profile))
            {
                Profile.Name = ProfileName;
                Profile.CollisionEnabled = ECollisionEnabled::QueryOnly;
                Profile.ObjectType = ObjectType;
                Profile.ObjectTypeName = SensorChannelName;
                Profile.bCanModify = false;
                Profile.ResponseToChannels = FCollisionResponseContainer::GetDefaultResponseContainer();
                Profile.ResponseToChannels.Visibility = ECR_Ignore;
                Profile.ResponseToChannels.Camera = ECR_Ignore;
                Profile.ResponseToChannels.WorldStatic = ECR_Overlap;
                Profile.ResponseToChannels.WorldDynamic = ECR_Ignore;
                Profile.ResponseToChannels.Pawn = ECR_Ignore;
                Profile.ResponseToChannels.PhysicsBody = ECR_Ignore;
                Profile.ResponseToChannels.Vehicle = ECR_Ignore;
                Profile.ResponseToChannels.Destructible = ECR_Ignore;
                Profile.ResponseToChannels.EnumArray[SensorChannelIndex] = ECR_Ignore;
                Profile.ResponseToChannels.EnumArray[MarkerChannelIndex] = ECR_Overlap;
#if WITH_EDITORONLY_DATA
                Profile.HelpMessage = TEXT("Sensor detects Markers and WorldStatic");
#endif
            }

            FCollisionProfilePrivateAccessor::AddProfileTemplate(Profile);
        }

        {
            FCollisionResponseTemplate Profile;
            const auto ObjectType = TEnumAsByte<ECollisionChannel>{MarkerChannelIndex};
            if (NOT InCollisionProfile->GetProfileTemplate(MarkerChannelName, Profile))
            {
                Profile.Name = MarkerChannelName;
                Profile.CollisionEnabled = ECollisionEnabled::QueryOnly;
                Profile.ObjectType = ObjectType;
                Profile.ObjectTypeName = MarkerChannelName;
                Profile.bCanModify = false;
                Profile.ResponseToChannels = FCollisionResponseContainer::GetDefaultResponseContainer();
                Profile.ResponseToChannels.Visibility = ECR_Ignore;
                Profile.ResponseToChannels.Camera = ECR_Ignore;
                Profile.ResponseToChannels.WorldStatic = ECR_Ignore;
                Profile.ResponseToChannels.WorldDynamic = ECR_Ignore;
                Profile.ResponseToChannels.Pawn = ECR_Ignore;
                Profile.ResponseToChannels.PhysicsBody = ECR_Ignore;
                Profile.ResponseToChannels.Vehicle = ECR_Ignore;
                Profile.ResponseToChannels.Destructible = ECR_Ignore;
                Profile.ResponseToChannels.EnumArray[SensorChannelIndex] = ECR_Overlap;
                Profile.ResponseToChannels.EnumArray[MarkerChannelIndex] = ECR_Ignore;
#if WITH_EDITORONLY_DATA
                Profile.HelpMessage = TEXT("Markers are detected by Sensors");
#endif
            }

            FCollisionProfilePrivateAccessor::AddProfileTemplate(Profile);
        }
    };

    FCoreDelegates::OnFEngineLoopInitComplete.AddLambda([SetupCollisionProfiles]()
    {
        UCollisionProfile::Get()->OnLoadProfileConfig.AddLambda([SetupCollisionProfiles](UCollisionProfile* InCollisionProfile)
        {
            SetupCollisionProfiles(InCollisionProfile, 1.0f);
        });

        SetupCollisionProfiles(UCollisionProfile::Get(), 1.0f);
    });
}

auto
    FCkOverlapBodyEditorModule::
    ShutdownModule()
    -> void
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkOverlapBodyEditorModule, CkOverlapBodyEditor)
