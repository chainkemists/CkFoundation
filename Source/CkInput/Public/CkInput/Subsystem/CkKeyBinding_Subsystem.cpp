#include "CkKeyBinding_Subsystem.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkInput/CkKeyBinding_Utils.h"
#include "CkInput/Settings/CkInput_Settings.h"

#include <AssetRegistry/IAssetRegistry.h>
#include <EnhancedInputSubsystems.h>
#include <InputMappingContext.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_KeyBinding_Subsystem::
    Initialize(
        FSubsystemCollectionBase& Collection)
    -> void
{
    Super::Initialize(Collection);

    const auto* LocalPlayer = GetLocalPlayer();
    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* EISubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
    if (ck::Is_NOT_Valid(EISubsystem))
    { return; }

    auto* Settings = EISubsystem->GetUserSettings();
    if (ck::Is_NOT_Valid(Settings))
    { return; }

    Settings->OnSettingsChanged.AddDynamic(this, &UCk_KeyBinding_Subsystem::OnSettingsChanged);
    _Bound = true;

    // Register all IMCs from configured scan paths so their mapping rows
    // exist in the key profile before any remap attempt.
    {
        if (const auto& ScanPaths = UCk_Utils_Input_Settings_UE::Get_MappingContextScanPaths();
            NOT ScanPaths.IsEmpty())
        {
            if (auto* AssetRegistry = IAssetRegistry::Get();
                ck::IsValid(AssetRegistry, ck::IsValid_Policy_NullptrOnly{}))
            {
                auto Filter = FARFilter{};
                Filter.ClassPaths.Add(UInputMappingContext::StaticClass()->GetClassPathName());
                Filter.bRecursivePaths = true;

                for (const auto& DirPath : ScanPaths)
                {
                    Filter.PackagePaths.Add(FName(*DirPath.Path));
                }

                auto Assets = TArray<FAssetData>{};
                AssetRegistry->GetAssets(Filter, Assets);

                for (const auto& AssetData : Assets)
                {
                    if (const auto* IMC = Cast<UInputMappingContext>(AssetData.GetAsset());
                        ck::IsValid(IMC))
                    {
                        Settings->RegisterInputMappingContext(IMC);
                    }
                }
            }
        }
    }
}

auto
    UCk_KeyBinding_Subsystem::
    Deinitialize()
    -> void
{
    if (_Bound)
    {
        if (const auto* LocalPlayer = GetLocalPlayer(); ck::IsValid(LocalPlayer))
        {
            if (auto* EISubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
                ck::IsValid(EISubsystem))
            {
                if (auto* Settings = EISubsystem->GetUserSettings();
                    ck::IsValid(Settings))
                {
                    Settings->OnSettingsChanged.RemoveDynamic(this, &UCk_KeyBinding_Subsystem::OnSettingsChanged);
                }
            }
        }

        _Bound = false;
    }

    _Watchers.Empty();

    Super::Deinitialize();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_KeyBinding_Subsystem::
    BindTo_MappingKeyChanged(
        const FCk_Handle_KeybindListener& Listener)
    -> void
{
    auto& Entry = _Watchers.AddDefaulted_GetRef();
    Entry.MappingName = Listener.Get_MappingName();
    Entry.Slot = Listener.Get_Slot();
    Entry.OnChanged = Listener.Get_OnChanged();

    // Cache the current key so we can detect changes later.
    if (const auto* LocalPlayer = GetLocalPlayer();
        ck::IsValid(LocalPlayer))
    {
        if (auto* PC = LocalPlayer->GetPlayerController(GetWorld());
            ck::IsValid(PC))
        {
            Entry.CachedKey = UCk_Utils_KeyBinding_UE::Get_KeyForMapping(PC, Entry.MappingName, Entry.Slot);
        }
    }
}

auto
    UCk_KeyBinding_Subsystem::
    UnbindFrom_MappingKeyChanged(
        const FCk_Handle_KeybindListener& Listener)
    -> void
{
    const auto MappingName = Listener.Get_MappingName();
    const auto Slot = Listener.Get_Slot();

    _Watchers.RemoveAll([&](const FWatcherEntry& Entry)
    {
        return Entry.MappingName == MappingName && Entry.Slot == Slot;
    });
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_KeyBinding_Subsystem::
    OnSettingsChanged(
        UEnhancedInputUserSettings* Settings)
    -> void
{
    const auto* LocalPlayer = GetLocalPlayer();
    if (ck::Is_NOT_Valid(LocalPlayer))
    { return; }

    auto* PC = LocalPlayer->GetPlayerController(GetWorld());
    if (ck::Is_NOT_Valid(PC))
    { return; }

    for (auto& [MappingName, Slot, CachedKey, OnChanged] : _Watchers)
    {
        const auto NewKey = UCk_Utils_KeyBinding_UE::Get_KeyForMapping(PC, MappingName, Slot);
        if (NewKey == CachedKey)
        { continue; }

        const auto OldKey = CachedKey;
        CachedKey = NewKey;

        OnChanged.ExecuteIfBound(MappingName, OldKey, NewKey);
    }
}

// --------------------------------------------------------------------------------------------------------------------
