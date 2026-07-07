#include "CkUsfEditor_Module.h"

#include "CkUsfEditor/Generator/CkUsf_Generator.h"
#include "CkUsfEditor_Log.h"

#include "CkUsf/LookDefinition/CkUsf_LookDefinition.h"

#include "CkCore/Macros/CkMacros.h"

#include "CoreGlobals.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"
#include "UObject/UObjectHash.h"

#define LOCTEXT_NAMESPACE "FCkUsfEditorModule"

void FCkUsfEditorModule::StartupModule()
{
    // Auto-regen on asset save: saving a LookDefinition refreshes its master, so the authoring loop
    // is "save asset → master regens" with no manual step. Interactive editor only — commandlets
    // (cook, resave) must not churn generated masters as a save side-effect.
    if (NOT IsRunningCommandlet())
    {
        _PackageSavedHandle = UPackage::PackageSavedWithContextEvent.AddRaw(
            this, &FCkUsfEditorModule::OnPackageSaved);
    }
}

void FCkUsfEditorModule::ShutdownModule()
{
    if (_PackageSavedHandle.IsValid())
    {
        UPackage::PackageSavedWithContextEvent.Remove(_PackageSavedHandle);
        _PackageSavedHandle.Reset();
    }
    if (_PendingTicker.IsValid())
    {
        FTSTicker::GetCoreTicker().RemoveTicker(_PendingTicker);
        _PendingTicker.Reset();
    }
}

void FCkUsfEditorModule::OnPackageSaved(
    const FString& InFilename, UPackage* InPackage, FObjectPostSaveContext InContext)
{
    // Procedural saves (cook, bulk resave) are not authoring; regenerating there would churn assets.
    if (InContext.IsProceduralSave() || InPackage == nullptr)
    { return; }

    TArray<UObject*> Objects;
    constexpr auto IncludeNestedObjects = false;
    GetObjectsWithPackage(InPackage, Objects, IncludeNestedObjects);

    auto QueuedAny = false;
    for (auto* Object : Objects)
    {
        auto* Def = Cast<UCkUsf_LookDefinition>(Object);
        if (Def == nullptr)
        { continue; }

        _PendingRegens.AddUnique(Def);
        QueuedAny = true;
    }

    // Defer to the next tick: regeneration saves the master package, and saving from inside another
    // package's save callback is asking for reentrancy trouble.
    if (QueuedAny && NOT _PendingTicker.IsValid())
    {
        _PendingTicker = FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateRaw(this, &FCkUsfEditorModule::DoDrainPendingRegens));
    }
}

bool FCkUsfEditorModule::DoDrainPendingRegens(float InDeltaT)
{
    const auto Pending = MoveTemp(_PendingRegens);
    _PendingRegens.Reset();
    _PendingTicker.Reset();

    for (const auto& WeakDef : Pending)
    {
        if (auto* Def = WeakDef.Get())
        {
            ck::usf_editor::Log(TEXT("LookDefinition [{}] saved — regenerating its master"),
                Def->Get_EffectiveLookName());
            ck::usf_editor::Generate_LookMaterial(Def);
        }
    }

    constexpr auto KeepTicking = false;   // one-shot: re-registered by the next qualifying save
    return KeepTicking;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkUsfEditorModule, CkUsfEditor)
