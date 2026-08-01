#include "CkAssetExporter_AutoReimportGuard.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Factories/Factory.h"
#include "FileCacheUtilities.h"
#include "Settings/EditorLoadingSavingSettings.h"

#include <CoreGlobals.h>
#include <UObject/UObjectIterator.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_autoreimport_guard
{
    struct FEffectiveMonitor
    {
        // Parsed: what the monitor actually watches, used for the same-or-parent dedup below.
        FString SourceDirectory;
        // Authored: ParseSourceDirectoryAndMountPoint rewrites "/Game/" to "../../Content/", so echoing the parsed
        // form back at the user would tell them to add a config line that does not match the one they already have.
        FString AuthoredSourceDirectory;
        DirectoryWatcher::FMatchRules Rules;
    };

    // Reproduces SetUpDirectoryMonitors: parse each configured entry, stamp the factory extensions, append the
    // wildcard rules, then drop any entry a LATER entry supersedes (StartsWith is true for identical paths, so the
    // last entry naming a directory wins). Testing anything but the surviving set would report rules that never run.
    static auto
    Build_EffectiveMonitors(
        const UEditorLoadingSavingSettings& InSettings,
        const FString& InSupportedExtensions)
        -> TArray<FEffectiveMonitor>
    {
        auto Parsed = TArray<FEffectiveMonitor>{};

        for (const auto& Setting : InSettings.AutoReimportDirectorySettings)
        {
            auto Entry = FEffectiveMonitor{};
            Entry.SourceDirectory = Setting.SourceDirectory;
            Entry.AuthoredSourceDirectory = Setting.SourceDirectory;

            auto MountPoint = Setting.MountPoint;

            constexpr auto EnableLogging = false;
            const auto ParseContext = FAutoReimportDirectoryConfig::FParseContext{EnableLogging};
            if (NOT FAutoReimportDirectoryConfig::ParseSourceDirectoryAndMountPoint(
                Entry.SourceDirectory, MountPoint, ParseContext))
            { continue; }

            Entry.Rules.SetApplicableExtensions(InSupportedExtensions);
            for (const auto& Wildcard : Setting.Wildcards)
            { Entry.Rules.AddWildcardRule(Wildcard.Wildcard, Wildcard.bInclude); }

            Parsed.Add(MoveTemp(Entry));
        }

        auto Effective = TArray<FEffectiveMonitor>{};

        for (auto Index = int32{0}; Index < Parsed.Num(); ++Index)
        {
            auto Superseded = false;
            for (auto OtherIndex = Index + 1; OtherIndex < Parsed.Num(); ++OtherIndex)
            {
                if (Parsed[Index].SourceDirectory.StartsWith(Parsed[OtherIndex].SourceDirectory))
                {
                    Superseded = true;
                    break;
                }
            }

            if (Superseded)
            { continue; }

            Effective.Add(Parsed[Index]);
        }

        return Effective;
    }
}

// --------------------------------------------------------------------------------------------------------------------

// Mirrors FAutoReimportManager::GetAllFactoryExtensions, which is a PRIVATE static we cannot call. Kept deliberately
// identical (including the ';'-terminated accumulation shape MatchExtensionString consumes) so this guard reports what
// the monitor actually does rather than an approximation of it.
auto
    FCk_AssetExporter_AutoReimportGuard::
    Get_AllFactoryExtensions()
    -> FString
{
    auto AllExtensions = FString{};

    auto Scratch = FString{};
    Scratch.Reserve(32);

    for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
    {
        auto* Class = *ClassIt;

        if (NOT Class->IsChildOf(UFactory::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        const auto* Factory = Cast<UFactory>(Class->GetDefaultObject());
        if (ck::Is_NOT_Valid(Factory, ck::IsValid_Policy_NullptrOnly{}) || NOT Factory->bEditorImport)
        { continue; }

        for (const auto& Format : Factory->Formats)
        {
            auto SeparatorIndex = int32{INDEX_NONE};
            if (NOT Format.FindChar(TEXT(';'), SeparatorIndex) || SeparatorIndex <= 0)
            { continue; }

            Scratch.GetCharArray().Reset();
            Scratch.AppendChars(*Format, SeparatorIndex + 1);

            if (AllExtensions.Find(Scratch) == INDEX_NONE)
            { AllExtensions += Scratch; }
        }
    }

    return AllExtensions;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_AutoReimportGuard::
    Validate()
    -> void
{
    using namespace ck_asset_exporter_autoreimport_guard;

    if (IsRunningCommandlet())
    { return; }

    const auto* Settings = GetDefault<UEditorLoadingSavingSettings>();
    if (ck::Is_NOT_Valid(Settings, ck::IsValid_Policy_NullptrOnly{}) || NOT Settings->bMonitorContentDirectories)
    { return; }

    const auto SupportedExtensions = FCk_AssetExporter_AutoReimportGuard::Get_AllFactoryExtensions();
    const auto EffectiveMonitors = Build_EffectiveMonitors(*Settings, SupportedExtensions);

    // The .csv a DataTable export writes IS a legitimate re-import source for that DataTable, so a monitor picking it
    // up is correct behaviour and must not be reported. Only the two summary/structured siblings are pure output.
    const auto ProbedExtensions = TArray<FString>
    {
        ck::asset_exporter::extension::Sidecar,
        ck::asset_exporter::extension::SummaryText,
    };

    for (const auto& Monitor : EffectiveMonitors)
    {
        for (const auto& Extension : ProbedExtensions)
        {
            const auto ProbeFilename = FString{TEXT("CkAssetExporterProbe")} + Extension;
            if (NOT Monitor.Rules.IsFileApplicable(*ProbeFilename))
            { continue; }

            ck::asset_exporter::Warning(
                TEXT("[AutoReimportGuard] The auto-reimport monitor watching [{}] will pick up the [{}] files this ")
                TEXT("module writes next to assets under Content/. Those are export output, never import sources. ")
                TEXT("Once enough accumulate the monitor parks on its \"N changes to source content files\" prompt ")
                TEXT("and re-stats the whole pending set every frame -- measured at 18 fps on a 1,178-file corpus, ")
                TEXT("invisible to every UE-side profiler. Exclude it in Config/DefaultEditorPerProjectUserSettings.ini ")
                TEXT("(bInclude defaults to false, so a bare Wildcard entry IS an exclusion):\n")
                TEXT("+AutoReimportDirectorySettings=(SourceDirectory=\"{}\",MountPoint=\"\",Wildcards=((Wildcard=\"*{}\")))"),
                Monitor.SourceDirectory,
                Extension,
                Monitor.AuthoredSourceDirectory,
                Extension);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
