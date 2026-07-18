#include "CkAssetExporter_RequestLoop.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/Dispatch/CkAssetExporter_Dispatch.h"

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "AssetRegistry/AssetRegistryModule.h"

#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <HAL/FileManager.h>
#include <HAL/PlatformProcess.h>
#include <HAL/PlatformTime.h>
#include <Misc/DateTime.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <UObject/UObjectGlobals.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_asset_exporter_requestloop
{
    constexpr auto IdleTimeoutSeconds = double{10.0 * 60.0};
    constexpr auto MaxRuntimeSeconds  = double{2.0 * 60.0 * 60.0};
    constexpr auto PollSeconds        = float{0.5f};

    static auto
    Write_Json(
        const TSharedPtr<FJsonObject>& InObject,
        const FString& InPath)
        -> void
    {
        auto JsonString = FString{};
        const auto Writer = TJsonWriterFactory<>::Create(&JsonString);
        FJsonSerializer::Serialize(InObject.ToSharedRef(), Writer);
        FFileHelper::SaveStringToFile(JsonString, *InPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    }

    static auto
    Write_Error(
        const FString& InResultPath,
        const FString& InMessage)
        -> void
    {
        auto Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), false);
        Result->SetStringField(TEXT("error"), InMessage);
        Write_Json(Result, InResultPath);
    }

    static auto
    Read_StringArray(
        const TSharedPtr<FJsonObject>& InObject,
        const FString& InField,
        TArray<FString>& OutArray)
        -> void
    {
        const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
        if (NOT InObject->TryGetArrayField(InField, Values) || Values == nullptr)
        { return; }

        for (const auto& Value : *Values)
        {
            auto Str = FString{};
            if (Value.IsValid() && Value->TryGetString(Str))
            { OutArray.Add(Str); }
        }
    }

    static auto
    Merge_Summary(
        FCk_AssetExportDispatchSummary& InOutSummary,
        const FCk_AssetExportDispatchSummary& InSummary)
        -> void
    {
        InOutSummary.Succeeded = InOutSummary.Succeeded && InSummary.Succeeded;
        if (NOT InSummary.ErrorMessage.IsEmpty() && InOutSummary.ErrorMessage.IsEmpty())
        { InOutSummary.ErrorMessage = InSummary.ErrorMessage; }
        InOutSummary.Exported += InSummary.Exported;
        InOutSummary.Skipped  += InSummary.Skipped;
        InOutSummary.Failed   += InSummary.Failed;
        InOutSummary.Entries.Append(InSummary.Entries);
    }

    static auto
    Delete_FilesIn(
        const FString& InDir,
        const TCHAR* InWildcard)
        -> void
    {
        auto& FileManager = IFileManager::Get();
        auto FoundFiles = TArray<FString>{};
        constexpr auto FindFiles = true;
        constexpr auto FindDirectories = false;
        FileManager.FindFiles(FoundFiles, *FPaths::Combine(InDir, InWildcard), FindFiles, FindDirectories);

        for (const auto& FileName : FoundFiles)
        {
            constexpr auto RequireExists = false;
            FileManager.Delete(*FPaths::Combine(InDir, FileName), RequireExists);
        }
    }

    // ---- op handlers ----

    static auto
    Handle_Export(
        const TSharedPtr<FJsonObject>& InRequest,
        const FString& InResultPath)
        -> void
    {
        auto Assets = TArray<FString>{};
        Read_StringArray(InRequest, TEXT("assets"), Assets);

        auto Dir = FString{};
        InRequest->TryGetStringField(TEXT("dir"), Dir);

        auto Classes = TArray<FString>{};
        Read_StringArray(InRequest, TEXT("classes"), Classes);

        auto SkipFresh = false;
        InRequest->TryGetBoolField(TEXT("skipFresh"), SkipFresh);
        auto Force = false;
        InRequest->TryGetBoolField(TEXT("force"), Force);
        const auto EffectiveSkipFresh = SkipFresh && NOT Force;

        auto Summary = FCk_AssetExportDispatchSummary{};
        Summary.Succeeded = true;

        if (Assets.Num() > 0)
        {
            const auto ObjectPaths = FCk_AssetExporter_Dispatch::Resolve_TokensToObjectPaths(Assets, Summary);
            Merge_Summary(Summary, FCk_AssetExporter_Dispatch::ExportAssets(ObjectPaths, EffectiveSkipFresh));
        }

        if (NOT Dir.IsEmpty())
        { Merge_Summary(Summary, FCk_AssetExporter_Dispatch::SweepDirectory(Dir, Classes, EffectiveSkipFresh)); }

        auto Result = MakeShared<FJsonObject>();
        Result->SetBoolField(TEXT("ok"), Summary.Succeeded && Summary.Failed == 0);
        if (NOT Summary.ErrorMessage.IsEmpty())
        { Result->SetStringField(TEXT("error"), Summary.ErrorMessage); }

        auto PerAsset = TArray<TSharedPtr<FJsonValue>>{};
        for (const auto& Entry : Summary.Entries)
        {
            auto Obj = MakeShared<FJsonObject>();
            Obj->SetStringField(TEXT("asset"), Entry.AssetPath);
            Obj->SetBoolField(TEXT("ok"), Entry.Succeeded);
            Obj->SetBoolField(TEXT("skipped"), Entry.Skipped);
            Obj->SetStringField(TEXT("output"), Entry.JsonFilePath.IsEmpty() ? Entry.JsonFilePath : FPaths::ConvertRelativePathToFull(Entry.JsonFilePath));
            Obj->SetStringField(TEXT("error"), Entry.ErrorMessage);
            PerAsset.Add(MakeShared<FJsonValueObject>(Obj));
        }
        Result->SetArrayField(TEXT("perAsset"), PerAsset);

        Write_Json(Result, InResultPath);

        ck::asset_exporter::Display(
            TEXT("[Server] export: {} exported, {} skipped, {} failed"),
            Summary.Exported, Summary.Skipped, Summary.Failed);
    }

    static auto
    Handle_List(
        const TSharedPtr<FJsonObject>& InRequest,
        const FString& InResultPath)
        -> void
    {
        auto Dir = FString{};
        InRequest->TryGetStringField(TEXT("dir"), Dir);

        if (Dir.IsEmpty())
        {
            Write_Error(InResultPath, TEXT("list op requires a 'dir' field"));
            return;
        }

        auto Classes = TArray<FString>{};
        Read_StringArray(InRequest, TEXT("classes"), Classes);

        const auto ListJson = FCk_AssetExporter_Dispatch::ListAssets(Dir, Classes);

        auto Result = MakeShared<FJsonObject>();
        auto Ok = false;
        ListJson->TryGetBoolField(TEXT("ok"), Ok);
        Result->SetBoolField(TEXT("ok"), Ok);

        const TArray<TSharedPtr<FJsonValue>>* Assets = nullptr;
        if (ListJson->TryGetArrayField(TEXT("assets"), Assets) && Assets != nullptr)
        { Result->SetArrayField(TEXT("assets"), *Assets); }

        auto Error = FString{};
        if (ListJson->TryGetStringField(TEXT("error"), Error))
        { Result->SetStringField(TEXT("error"), Error); }

        Write_Json(Result, InResultPath);
    }

    // Returns true when the request asked the server to quit.
    static auto
    Process_Request(
        const FString& InRequestPath,
        const FString& InResultPath)
        -> bool
    {
        auto JsonString = FString{};
        if (NOT FFileHelper::LoadFileToString(JsonString, *InRequestPath))
        {
            Write_Error(InResultPath, TEXT("Could not read request file"));
            return false;
        }

        auto Request = TSharedPtr<FJsonObject>{};
        const auto Reader = TJsonReaderFactory<>::Create(JsonString);
        if (NOT FJsonSerializer::Deserialize(Reader, Request) || NOT Request.IsValid())
        {
            Write_Error(InResultPath, TEXT("Malformed request json"));
            return false;
        }

        auto Op = FString{};
        Request->TryGetStringField(TEXT("op"), Op);

        if (Op.Equals(TEXT("quit"), ESearchCase::IgnoreCase))
        {
            auto Result = MakeShared<FJsonObject>();
            Result->SetBoolField(TEXT("ok"), true);
            Write_Json(Result, InResultPath);
            ck::asset_exporter::Display(TEXT("[Server] quit requested"));
            return true;
        }

        if (Op.Equals(TEXT("list"), ESearchCase::IgnoreCase))
        {
            Handle_List(Request, InResultPath);
            return false;
        }

        if (Op.Equals(TEXT("export"), ESearchCase::IgnoreCase))
        {
            Handle_Export(Request, InResultPath);
            return false;
        }

        Write_Error(InResultPath, ck::Format_UE(TEXT("Unknown op [{}] (expected export | list | quit)"), Op));
        return false;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExporter_RequestLoop::
    Run()
    -> int32
{
    using namespace ck_asset_exporter_requestloop;

    // Absolute on purpose: these paths are handed to EXTERNAL processes via server.json — a relative
    // "../../Saved/..." only resolves from the editor's own cwd, not the submitter's.
    const auto Root         = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("CkAssetExporter")));
    const auto RequestsDir  = FPaths::Combine(Root, TEXT("Requests"));
    const auto ResultsDir   = FPaths::Combine(Root, TEXT("Results"));
    const auto ServerStatus = FPaths::Combine(Root, TEXT("server.json"));

    auto& FileManager = IFileManager::Get();

    constexpr auto MakeTree = true;
    FileManager.MakeDirectory(*RequestsDir, MakeTree);
    FileManager.MakeDirectory(*ResultsDir, MakeTree);

    // Stale request files from a crashed prior run would otherwise be re-processed against whatever they asked for.
    Delete_FilesIn(RequestsDir, TEXT("*.json"));

    {
        auto& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        AssetRegistry.WaitForCompletion();
    }

    {
        auto Status = MakeShared<FJsonObject>();
        Status->SetNumberField(TEXT("pid"), static_cast<double>(FPlatformProcess::GetCurrentProcessId()));
        Status->SetStringField(TEXT("startedAt"), FDateTime::UtcNow().ToIso8601());
        Status->SetStringField(TEXT("project"), FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath()));
        Status->SetStringField(TEXT("requestsDir"), RequestsDir);
        Status->SetStringField(TEXT("resultsDir"), ResultsDir);
        Write_Json(Status, ServerStatus);
    }

    ck::asset_exporter::Display(
        TEXT("[Server] ready — pid {}, polling [{}] every 500ms"),
        FPlatformProcess::GetCurrentProcessId(), RequestsDir);

    const auto StartSeconds = FPlatformTime::Seconds();
    auto LastActivitySeconds = StartSeconds;
    auto Quit = false;

    while (NOT Quit)
    {
        auto RequestFiles = TArray<FString>{};
        constexpr auto FindFiles = true;
        constexpr auto FindDirectories = false;
        FileManager.FindFiles(RequestFiles, *FPaths::Combine(RequestsDir, TEXT("*.json")), FindFiles, FindDirectories);
        RequestFiles.Sort();

        if (RequestFiles.Num() == 0)
        {
            const auto Now = FPlatformTime::Seconds();
            if (Now - LastActivitySeconds >= IdleTimeoutSeconds)
            {
                ck::asset_exporter::Display(TEXT("[Server] idle {} min with no requests — clean exit"), IdleTimeoutSeconds / 60.0);
                break;
            }
            if (Now - StartSeconds >= MaxRuntimeSeconds)
            {
                ck::asset_exporter::Display(TEXT("[Server] {}h wall-clock cap reached — clean exit"), MaxRuntimeSeconds / 3600.0);
                break;
            }
            FPlatformProcess::Sleep(PollSeconds);
            continue;
        }

        for (const auto& RequestFileName : RequestFiles)
        {
            const auto RequestPath = FPaths::Combine(RequestsDir, RequestFileName);
            const auto ResultPath  = FPaths::Combine(ResultsDir, FPaths::GetBaseFilename(RequestFileName) + TEXT(".json"));

            // A request seen the instant it appears may still be mid-write by the submitter (writers hold the file
            // with shared-read access, so a premature read sees truncated json and the request would be consumed as
            // "malformed" — a lost request). Skip sub-second-old files; the next poll tick picks them up settled.
            const auto FileAge = FDateTime::UtcNow() - FileManager.GetTimeStamp(*RequestPath);
            if (FileAge < FTimespan::FromSeconds(1.0))
            { continue; }

            const auto ShouldQuit = Process_Request(RequestPath, ResultPath);

            // Delete the request only AFTER the result is written, so an external watcher never sees a consumed
            // request with no result.
            constexpr auto RequireExists = false;
            FileManager.Delete(*RequestPath, RequireExists);

            CollectGarbage(RF_NoFlags);
            LastActivitySeconds = FPlatformTime::Seconds();

            if (ShouldQuit)
            { Quit = true; break; }

            if (FPlatformTime::Seconds() - StartSeconds >= MaxRuntimeSeconds)
            {
                ck::asset_exporter::Display(TEXT("[Server] {}h wall-clock cap reached — clean exit"), MaxRuntimeSeconds / 3600.0);
                Quit = true;
                break;
            }
        }
    }

    // On EVERY exit path the status file is removed so external drivers can tell the server is down.
    constexpr auto RequireExists = false;
    FileManager.Delete(*ServerStatus, RequireExists);

    ck::asset_exporter::Display(TEXT("[Server] stopped"));
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
