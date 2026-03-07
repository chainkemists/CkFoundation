#include "CkWatermark_Settings.h"

#include "CkWatermark/Subsystem/CkWatermark_Subsystem.h"

#include <Engine/Engine.h>
#include <Engine/GameInstance.h>
#include <Engine/LocalPlayer.h>
#include <Engine/World.h>

// --------------------------------------------------------------------------------------------------------------------

UCk_Watermark_ProjectSettings_UE::UCk_Watermark_ProjectSettings_UE(
    const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    // Ensure Count (non-unique): green at 0, yellow single-digit, orange 10-25, red 26+
    _Watermark_EnsureCount_ColorBands.VeryGood_Threshold = 0.f;
    _Watermark_EnsureCount_ColorBands.Good_Threshold     = 5.f;
    _Watermark_EnsureCount_ColorBands.Okay_Threshold     = 9.f;
    _Watermark_EnsureCount_ColorBands.Bad_Threshold      = 25.f;

    // Unique Ensure Count: green at 0, yellow single-digit, red at 10+
    _Watermark_UniqueEnsureCount_ColorBands.VeryGood_Threshold = 0.f;
    _Watermark_UniqueEnsureCount_ColorBands.Good_Threshold     = 4.f;
    _Watermark_UniqueEnsureCount_ColorBands.Okay_Threshold     = 9.f;
    _Watermark_UniqueEnsureCount_ColorBands.Bad_Threshold      = 9.f;  // same as Okay → no orange band
}

// --------------------------------------------------------------------------------------------------------------------

void UCk_Watermark_ProjectSettings_UE::ForceRebuildWatermark()
{
    if (!GEngine) { return; }

    for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
    {
        const UWorld* World = Ctx.World();
        if (!World) { continue; }

        const UGameInstance* GI = World->GetGameInstance();
        if (!GI) { continue; }

        for (ULocalPlayer* LP : GI->GetLocalPlayers())
        {
            if (UCk_Watermark_Subsystem_UE* Sub = LP->GetSubsystem<UCk_Watermark_Subsystem_UE>())
            {
                Sub->ForceRebuildWidget();
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_FPS_ColorBands()
    -> const FCk_Watermark_ColorBands_HigherIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_FPS_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ServerFPS_ColorBands()
    -> const FCk_Watermark_ColorBands_HigherIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ServerFPS_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Ping_ColorBands()
    -> const FCk_Watermark_ColorBands_LowerIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Ping_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_EnsureCount_ColorBands()
    -> const FCk_Watermark_ColorBands_LowerIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_EnsureCount_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_UniqueEnsureCount_ColorBands()
    -> const FCk_Watermark_ColorBands_LowerIsBetter&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_UniqueEnsureCount_ColorBands();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_FontOverride()
    -> FSlateFontInfo
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_FontOverride();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ValueFontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ValueFontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_LabelFontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_LabelFontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BracketFontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BracketFontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildType_FontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildType_FontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_TextOutlineSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_TextOutlineSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_TextOutlineColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_TextOutlineColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_TextShadowOffset()
    -> FVector2D
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_TextShadowOffset();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_TextShadowColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_TextShadowColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_StatCell_HorizontalPadding()
    -> float
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_StatCell_HorizontalPadding();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_VerticalPadding()
    -> float
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_VerticalPadding();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Bracket_InnerPadding()
    -> float
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Bracket_InnerPadding();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BracketOpen()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BracketOpen();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BracketClose()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BracketClose();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_LabelColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_LabelColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_UncoloredStat_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_UncoloredStat_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Debug_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Debug_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Dev_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Dev_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Test_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Test_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildConfig_Shipping_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildConfig_Shipping_Color();
}

// ---- MinPolicy accessors (stat group) ----------------------------------------

auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Ensures()        -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Ensures(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_UniqueEnsures()   -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_UniqueEnsures(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Ram()             -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Ram(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Vram()            -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Vram(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_MemoryPressure()  -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_MemoryPressure(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Ping()            -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Ping(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_ServerFps()       -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_ServerFps(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Fps()             -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Fps(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Time()            -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Time(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Frame()           -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Frame(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_RepActors()       -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_RepActors(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_RepComponents()   -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_RepComponents(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_RepObjects()      -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_RepObjects(); }

// ---- MinPolicy accessors (info bar / build / ECS) ----------------------------

auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_BuildId()         -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_BuildId(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CpuBrand()        -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_CpuBrand(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_OsVersion()       -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_OsVersion(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CpuCores()        -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_CpuCores(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_NetworkType()     -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_NetworkType(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_NetMode()         -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_NetMode(); }

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_NetMode_Label_SinglePlayer()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_NetMode_Label_SinglePlayer();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_NetMode_Label_Server()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_NetMode_Label_Server();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_NetMode_Label_ListenServer()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_NetMode_Label_ListenServer();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_NetMode_Label_Client()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_NetMode_Label_Client();
}

auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_Jolt()             -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_Jolt(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsDebugger()     -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_EcsDebugger(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsEntityMap()    -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_EcsEntityMap(); }
auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_EcsCallstacks()   -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_EcsCallstacks(); }

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_Separator()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_Separator();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_KeyValueSeparator()
    -> FString
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_KeyValueSeparator();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_FontSize()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_FontSize();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_KeyColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_KeyColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_ValueColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_ValueColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_InfoBar_SeparatorColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_InfoBar_SeparatorColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildId_Active_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildId_Active_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildId_Inactive_Color()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildId_Inactive_Color();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_BuildId_EnabledBranches()
    -> const TArray<FString>&
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_BuildId_EnabledBranches();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Ensures()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Ensures();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_UniqueEnsures()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_UniqueEnsures();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Ram()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Ram();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Vram()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Vram();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_MemPressure()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_MemPressure();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Ping()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Ping();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_ServerFps()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_ServerFps();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Fps()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Fps();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Time()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Time();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_Frame()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_Frame();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_DefaultDisplayPolicy_Shipping()
    -> ECk_Watermark_DisplayPolicy
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_DefaultDisplayPolicy_Shipping();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Widget_ZOrder()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Widget_ZOrder();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_RepActors()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_RepActors();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_RepComponents()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_RepComponents();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_Row_RepObjects()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_Row_RepObjects();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ActivityBar_MaxHistory()
    -> int32
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ActivityBar_MaxHistory();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ActivityBar_ActiveColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ActivityBar_ActiveColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ActivityBar_HeldAccentColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ActivityBar_HeldAccentColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_ActivityBar_InactiveColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_ActivityBar_InactiveColor();
}

// ---- Custom Field accessors -------------------------------------------------

auto UCk_Utils_Watermark_ProjectSettings_UE::Get_Watermark_MinPolicy_CustomField()        -> ECk_Watermark_DisplayPolicy { return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_MinPolicy_CustomField(); }

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_CustomField_DefaultKeyColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_CustomField_DefaultKeyColor();
}

auto
    UCk_Utils_Watermark_ProjectSettings_UE::
    Get_Watermark_CustomField_DefaultValueColor()
    -> FLinearColor
{
    return GetDefault<UCk_Watermark_ProjectSettings_UE>()->Get_Watermark_CustomField_DefaultValueColor();
}

// --------------------------------------------------------------------------------------------------------------------
