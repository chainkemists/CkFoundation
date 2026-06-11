#include "CkAngelscriptEntityScriptParamsGenerator.h"

#include "CkAngelscriptGenerator/CkAngelscriptGenerator_Log.h"
#include "CkAngelscriptGenerator/CkAngelscriptGenerator_SharedUtils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Reflection/CkReflection_Utils.h"

#include "CkEcs/EntityScript/CkEntityScript.h"

#include <Engine/BlueprintGeneratedClass.h>
#include <HAL/FileManager.h>
#include <Interfaces/IPluginManager.h>
#include <Misc/App.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <UObject/Class.h>
#include <UObject/UObjectIterator.h>
#include <UObject/UnrealType.h>

#if WITH_ANGELSCRIPT_CK
#include "ClassGenerator/ASClass.h"
#endif

// --------------------------------------------------------------------------------------------------------------------

namespace ck_entity_script_params_generator
{
#if WITH_EDITOR

    // ---- Filtering -----------------------------------------------------

    auto Is_IncludedEntityScriptClass(UClass* InClass) -> bool
    {
        if (ck::Is_NOT_Valid(InClass, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        if (NOT InClass->IsChildOf(UCk_EntityScript_UE::StaticClass()))
        { return false; }

        if (InClass == UCk_EntityScript_UE::StaticClass())
        { return false; }

        // Only generate Params accessors for entity-script classes authored in code —
        // native C++ or AngelScript. Blueprint-asset entity scripts are excluded: their
        // spawn params are authored in the BP editor and are never referenced by a
        // static AS identifier, so a generated namespace block for them is dead weight.
        // NOTE: CLASS_CompiledFromBlueprint cannot be used here — the AS engine fork
        // sets that flag on AngelScript classes too. UASClass derives directly from
        // UClass, so only true BP assets are UBlueprintGeneratedClass.
        if (InClass->IsChildOf(UBlueprintGeneratedClass::StaticClass()))
        { return false; }

        constexpr auto DisqualifyingFlags =
            CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;

        if (InClass->HasAnyClassFlags(DisqualifyingFlags))
        { return false; }

        // Filter classes that have been torn down / are pending-kill. Without this, AS classes
        // that were deleted from source linger in TObjectIterator until GC runs and keep showing
        // up in the generated file even though the .as file no longer defines them.
        if (InClass->IsUnreachable() ||
            InClass->HasAnyFlags(RF_BeginDestroyed | RF_FinishDestroyed))
        { return false; }

#if WITH_ANGELSCRIPT_CK
        // For AS-defined classes, also check whether the source file still exists on disk.
        // When a user deletes a class from an .as file and the file still exists, the stale
        // UASClass hangs around; but if the whole file was deleted, that's a strong signal
        // the class is gone. (Detecting class-removal within a still-present file would
        // require re-parsing; leaving that to editor restart.)
        if (auto* ASClass = UASClass::GetFirstASClass(InClass))
        {
            if (ASClass->NewerVersion != nullptr)
            { return false; }

            const auto SourcePath = ASClass->GetSourceFilePath();
            if (NOT SourcePath.IsEmpty() && NOT FPaths::FileExists(SourcePath))
            { return false; }
        }
#endif

        if (UCk_Utils_Reflection_UE::Is_PlaceholderClass(InClass))
        { return false; }

        return true;
    }

    // ---- Block formatting ----------------------------------------------

    auto Is_ConstProperty(FProperty* InProperty) -> bool
    {
        return InProperty->HasAnyPropertyFlags(CPF_ConstParm | CPF_BlueprintReadOnly);
    }

    // A property's emission can produce two outputs:
    //   - DeclLine: the `UPROPERTY()\n    <Type> <Name>[ = <expr>];` field declaration.
    //   - OverrideStatements: `<Name>.<Path> = <Value>;` lines emitted into the SpawnParams
    //     default ctor body. Non-empty only for struct-typed properties whose CDO differs
    //     from struct default AND whose struct contains a UObject* field (which would
    //     otherwise trigger the positional-ctor `<null handle>` deadlock).
    struct FPropertyEmission
    {
        FString         DeclLine;
        TArray<FString> OverrideStatements;
    };

    auto Format_PropertyLine(FProperty* InProperty, const UClass* InClass) -> FPropertyEmission
    {
        auto AsType = FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(InProperty);
        if (Is_ConstProperty(InProperty) && NOT AsType.StartsWith(TEXT("const ")))
        {
            AsType = TEXT("const ") + AsType;
        }
        const auto& PropName = InProperty->GetName();

        auto Emission = FPropertyEmission{};
        Emission.DeclLine = ck::Format_UE(TEXT("    UPROPERTY()\n    {} {}"), AsType, PropName);

        const auto* CDO = InClass->GetDefaultObject();
        if (NOT ck::IsValid(CDO, ck::IsValid_Policy_NullptrOnly{}))
        {
            Emission.DeclLine += TEXT(";");
            return Emission;
        }

        // Field-assignment-style emission gate: struct-typed property whose struct contains
        // any UObject* (recursively) AND whose CDO differs from struct default. Without this
        // branch, the property would land as `<Type> <Name> = <Type>(arg1, ..., nullptr);`
        // which AS rejects with `No matching signatures to '<Type>(... <null handle>)'`.
        if (const auto* StructProp = CastField<FStructProperty>(InProperty))
        {
            if (auto* Struct = StructProp->Struct.Get();
                Struct != nullptr && UCk_Utils_Reflection_UE::Has_UObjectPointerField(Struct))
            {
                const auto Overrides = UCk_Utils_Reflection_UE::Get_StructFieldOverrides(StructProp, CDO);
                if (Overrides.Num() > 0)
                {
                    // Declare the field with no `= <expr>`; UScriptStruct::InitializeStruct
                    // zero-inits it (UObject* fields fall to their internal `= nullptr`
                    // declared defaults), and the SpawnParams default ctor body applies the
                    // diffs via dotted-path assignments below.
                    Emission.DeclLine += TEXT(";");
                    for (const auto& Override : Overrides)
                    {
                        const auto Expr = UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(Override._Literal);
                        if (Expr.IsEmpty())
                        { continue; }
                        Emission.OverrideStatements.Add(
                            ck::Format_UE(TEXT("        {}.{} = {};"), PropName, Override._DottedFieldPath, Expr));
                    }
                    return Emission;
                }
                // No diffs: fall through to the existing single-expression default emission,
                // which will produce `= <Type>()` (zero-arg ctor — safe).
            }
        }

        const auto Literal = UCk_Utils_Reflection_UE::Get_PropertyDefaultValueLiteral(InProperty, CDO);
        if (Literal.IsSet())
        {
            const auto DefaultExpr = UCk_Utils_Reflection_UE::Get_AngelscriptDefaultExpression(*Literal);
            if (NOT DefaultExpr.IsEmpty())
            {
                Emission.DeclLine += ck::Format_UE(TEXT(" = {}"), DefaultExpr);
            }
        }

        Emission.DeclLine += TEXT(";");
        return Emission;
    }

    // Builds a comma-separated `<Type> In<Name>` list of every valid exposed property,
    // for use in constructor signatures and the parameterized Params() overload.
    auto Format_ParameterList(const TArray<FProperty*>& InProps) -> FString
    {
        auto Out = FString{};
        for (auto Index = int32{0}; Index < InProps.Num(); ++Index)
        {
            auto AsType = FCkAngelscriptGenerator_SharedUtils::Get_DetailedPropertyType(InProps[Index]);
            if (Is_ConstProperty(InProps[Index]) && NOT AsType.StartsWith(TEXT("const ")))
            {
                AsType = TEXT("const ") + AsType;
            }
            const auto& PropName = InProps[Index]->GetName();
            Out += ck::Format_UE(TEXT("{} In{}"), AsType, PropName);
            if (Index + 1 < InProps.Num())
            { Out += TEXT(", "); }
        }
        return Out;
    }

    auto Format_ArgumentList(const TArray<FProperty*>& InProps) -> FString
    {
        auto Out = FString{};
        for (auto Index = int32{0}; Index < InProps.Num(); ++Index)
        {
            Out += ck::Format_UE(TEXT("In{}"), InProps[Index]->GetName());
            if (Index + 1 < InProps.Num())
            { Out += TEXT(", "); }
        }
        return Out;
    }

    auto Format_ClassBlock(UClass* InClass, int32& OutPropertyCount) -> FString
    {
        const auto AllProperties = UCk_Utils_Reflection_UE::Get_ExposedPropertiesOfClass(InClass);
        const auto ValidProps = ck::algo::Filter(AllProperties, [](FProperty* InProp)
        {
            return ck::IsValid(InProp, ck::IsValid_Policy_NullptrOnly{});
        });
        OutPropertyCount = ValidProps.Num();

        const auto ClassName = InClass->GetName();
        const auto FullClassName = ck::Format_UE(TEXT("{}{}"), InClass->GetPrefixCPP(), ClassName);
        const auto StructName = ck::Format_UE(TEXT("F{}_SpawnParams"), ClassName);

        auto Block = FString{};

        // File-scope struct with a unique USTRUCT name (avoids the "Params" name collision across
        // namespaces that trips the Unreal naming convention check).
        Block += TEXT("USTRUCT()\n");
        Block += ck::Format_UE(TEXT("struct {}\n"), StructName);
        Block += TEXT("{\n");

        auto AllOverrideStatements = TArray<FString>{};

        for (auto Index = int32{0}; Index < ValidProps.Num(); ++Index)
        {
            auto Emission = Format_PropertyLine(ValidProps[Index], InClass);
            Block += Emission.DeclLine;
            Block += TEXT("\n");
            if (Index + 1 < ValidProps.Num())
            { Block += TEXT("\n"); }
            AllOverrideStatements.Append(MoveTemp(Emission.OverrideStatements));
        }

        // Default constructor — only emit a body when at least one ExposeOnSpawn property
        // requires field-assignment-style override (struct field containing UObject* with CDO
        // diffs). For the common case (zero overrides) we keep emission byte-stable by
        // relying on the implicit zero-arg ctor.
        if (AllOverrideStatements.Num() > 0)
        {
            Block += TEXT("\n");
            Block += ck::Format_UE(TEXT("    {}()\n"), StructName);
            Block += TEXT("    {\n");
            for (const auto& Statement : AllOverrideStatements)
            {
                Block += Statement;
                Block += TEXT("\n");
            }
            Block += TEXT("    }\n");
        }

        // Parameterized constructor — only when there is at least one field to set.
        if (ValidProps.Num() > 0)
        {
            Block += TEXT("\n");
            Block += ck::Format_UE(TEXT("    {}({})\n"), StructName, Format_ParameterList(ValidProps));
            Block += TEXT("    {\n");
            for (const auto* Prop : ValidProps)
            {
                const auto& PropName = Prop->GetName();
                Block += ck::Format_UE(TEXT("        {} = In{};\n"), PropName, PropName);
            }
            Block += TEXT("    }\n");
        }

        Block += TEXT("}\n\n");

        // Namespace mirroring the class name so callers can still write
        // `UCk_MyEntityScript::Params(...)` — resolved to free function(s) returning the struct.
        Block += ck::Format_UE(TEXT("namespace {}\n"), FullClassName);
        Block += TEXT("{\n");

        // Default-constructed overload.
        Block += ck::Format_UE(TEXT("    {} Params()\n"), StructName);
        Block += TEXT("    {\n");
        Block += ck::Format_UE(TEXT("        return {}();\n"), StructName);
        Block += TEXT("    }\n");

        // All-fields overload — forwards to the parameterized struct constructor.
        if (ValidProps.Num() > 0)
        {
            Block += TEXT("\n");
            Block += ck::Format_UE(TEXT("    {} Params({})\n"), StructName, Format_ParameterList(ValidProps));
            Block += TEXT("    {\n");
            Block += ck::Format_UE(TEXT("        return {}({});\n"), StructName, Format_ArgumentList(ValidProps));
            Block += TEXT("    }\n");
        }

        Block += TEXT("}\n\n");

        return Block;
    }

    // ---- Plugin bucketing ----------------------------------------------

    struct FPluginBucket
    {
        FString PluginName;
        FString OutputFilePath;
        TArray<UClass*> Classes;
    };

    auto Build_ModuleToPluginMap() -> TMap<FString, TSharedRef<IPlugin>>
    {
        auto Result = TMap<FString, TSharedRef<IPlugin>>{};
        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        {
            for (const auto& Module : Plugin->GetDescriptor().Modules)
            {
                Result.Add(Module.Name.ToString(), Plugin);
            }
        }
        return Result;
    }

    auto Find_PluginByPathPrefix(const FString& InPath) -> const IPlugin*
    {
        if (InPath.IsEmpty())
        { return nullptr; }

        auto NormalizedPath = FPaths::ConvertRelativePathToFull(InPath);
        FPaths::NormalizeFilename(NormalizedPath);

        const IPlugin* BestMatch = nullptr;
        auto BestMatchLen = int32{0};

        for (const auto& Plugin : IPluginManager::Get().GetEnabledPlugins())
        {
            auto PluginDir = FPaths::ConvertRelativePathToFull(Plugin->GetBaseDir());
            FPaths::NormalizeDirectoryName(PluginDir);

            if (NormalizedPath.StartsWith(PluginDir, ESearchCase::IgnoreCase))
            {
                // Pick the longest match so nested plugin dirs win over ancestors.
                if (PluginDir.Len() > BestMatchLen)
                {
                    BestMatch = &Plugin.Get();
                    BestMatchLen = PluginDir.Len();
                }
            }
        }
        return BestMatch;
    }

    auto Find_PluginForClass(
        const UClass* InClass,
        const TMap<FString, TSharedRef<IPlugin>>& InModuleToPlugin)
        -> const IPlugin*
    {
        // Strategy 1: C++ class package name (/Script/ModuleName).
        // AS-defined classes all live under /Script/Angelscript so this only matches C++ classes.
        const auto PackageName = InClass->GetOutermost()->GetName();
        constexpr auto ScriptPrefix = TEXT("/Script/");
        if (PackageName.StartsWith(ScriptPrefix))
        {
            const auto ModuleName = PackageName.RightChop(FString(ScriptPrefix).Len());
            if (const auto* Found = InModuleToPlugin.Find(ModuleName))
            { return &Found->Get(); }
        }

#if WITH_ANGELSCRIPT_CK
        // Strategy 2: AS-defined classes expose their source .as file via UASClass.
        // GetFirstASClass walks the hierarchy and is more robust than a plain Cast.
        if (auto* ASClass = UASClass::GetFirstASClass(const_cast<UClass*>(InClass)))
        {
            if (const auto* Plugin = Find_PluginByPathPrefix(ASClass->GetSourceFilePath()))
            { return Plugin; }
        }
#endif

        return nullptr;
    }

    auto Bucket_ClassesByPlugin(const TArray<UClass*>& InClasses) -> TArray<FPluginBucket>
    {
        const auto ModuleToPlugin = Build_ModuleToPluginMap();

        auto BucketMap = TMap<FString, FPluginBucket>{};

        for (auto* Class : InClasses)
        {
            auto PluginName = FString{};
            auto OutputDir = FString{};

            if (const auto* Plugin = Find_PluginForClass(Class, ModuleToPlugin))
            {
                PluginName = Plugin->GetName();
                OutputDir = Plugin->GetBaseDir() / TEXT("Script") / TEXT("Generated");
            }
            else
            {
                // Not in any plugin — goes to the project bucket.
                PluginName = FApp::GetProjectName();
                OutputDir = FPaths::ProjectDir() / TEXT("Script") / TEXT("Generated");
            }

            if (NOT BucketMap.Contains(PluginName))
            {
                auto Bucket = FPluginBucket{};
                Bucket.PluginName = PluginName;
                Bucket.OutputFilePath = OutputDir / (PluginName + TEXT("_EntitySpawnParams.as"));
                BucketMap.Add(PluginName, MoveTemp(Bucket));
            }

            BucketMap[PluginName].Classes.Add(Class);
        }

        auto Result = TArray<FPluginBucket>{};
        BucketMap.GenerateValueArray(Result);
        Result.Sort([](const FPluginBucket& A, const FPluginBucket& B)
        {
            return A.PluginName < B.PluginName;
        });
        return Result;
    }

    // ---- File content --------------------------------------------------

    static const TCHAR* FileHeader =
        TEXT("// Auto-generated EntityScript spawn-params — DO NOT EDIT.\n")
        TEXT("// This file is regenerated on editor startup and after every AngelScript recompile.\n")
        TEXT("//\n")
        TEXT("// For each UCk_EntityScript_UE subclass, two declarations are emitted:\n")
        TEXT("//   - FCk_MyEntityScript_SpawnParams  (file-scope USTRUCT, unique name — avoids the\n")
        TEXT("//     `Params` name-collision across namespaces that trips the Unreal naming check)\n")
        TEXT("//   - namespace UCk_MyEntityScript { FCk_MyEntityScript_SpawnParams Params() { ... } }\n")
        TEXT("//     so callers can still write `UCk_MyEntityScript::Params()`.\n")
        TEXT("//\n")
        TEXT("// Properties are flattened across the hierarchy (AS has no struct inheritance). Non-\n")
        TEXT("// trivial struct defaults outside the CkReflection_Utils allowlist are emitted without\n")
        TEXT("// an initializer — set them on the instance before calling Request_SpawnEntity.\n\n");

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCkAngelscriptEntityScriptParamsGenerator::
    GenerateAll()
    -> void
{
#if WITH_EDITOR

    ck::angelscriptgenerator::Log(TEXT("[CkAS ES Params] === Generating EntityScript Spawn Params ==="));

    auto Classes = TArray<UClass*>{};
    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        auto* Class = *ClassIterator;
        if (ck_entity_script_params_generator::Is_IncludedEntityScriptClass(Class))
        {
            Classes.Add(Class);
        }
    }

    Classes.Sort([](const UClass& InA, const UClass& InB)
    {
        return InA.GetPathName() < InB.GetPathName();
    });

    const auto Buckets = ck_entity_script_params_generator::Bucket_ClassesByPlugin(Classes);

    auto TotalClasses = int32{0};
    auto TotalProperties = int32{0};

    for (const auto& Bucket : Buckets)
    {
        auto Content = FString{ck_entity_script_params_generator::FileHeader};

        auto BucketProperties = int32{0};
        for (auto* Class : Bucket.Classes)
        {
            auto PropertyCount = int32{0};
            Content += ck_entity_script_params_generator::Format_ClassBlock(Class, PropertyCount);
            BucketProperties += PropertyCount;
        }

        const auto OutputDir = FPaths::GetPath(Bucket.OutputFilePath);
        IFileManager::Get().MakeDirectory(*OutputDir, true);

        // Skip writing when the content hasn't changed — otherwise our own write triggers the
        // AngelScript PostCompile hook, which calls us again, ad infinitum.
        // Compare in LF-normalized form: Content is LF here (literals below), but the on-disk
        // file may be CRLF on Windows (see platform-native EOL step below), and
        // LoadFileToString normalizes to LF. Without normalizing both sides, a hand-edited or
        // externally-converted file (dos2unix / unix2dos / p4merge re-save) would trip an
        // unnecessary rewrite and re-enter the PostCompile hook.
        auto ExistingContent = FString{};
        const auto HasExisting = FFileHelper::LoadFileToString(ExistingContent, *Bucket.OutputFilePath);

        auto ContentForCompare = Content;
        ContentForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        auto ExistingForCompare = ExistingContent;
        ExistingForCompare.ReplaceInline(TEXT("\r\n"), TEXT("\n"));

        if (HasExisting && ExistingForCompare.Equals(ContentForCompare, ESearchCase::CaseSensitive))
        {
            ck::angelscriptgenerator::VeryVerbose(
                TEXT("[CkAS ES Params] [{}] up-to-date ({} classes)"),
                Bucket.PluginName, Bucket.Classes.Num());
            TotalClasses += Bucket.Classes.Num();
            TotalProperties += BucketProperties;
            continue;
        }

        // Rewrite-reason diagnostic. A rewrite of an unchanged class set should be rare —
        // every rewrite the AS file-watcher sees triggers a structural hot-reload and a
        // full soft-reload sweep AFTER engine init (multi-second editor freeze), so when
        // one happens we want the log to say exactly why.
        if (NOT HasExisting)
        {
            ck::angelscriptgenerator::Log(
                TEXT("[CkAS ES Params] [{}] rewrite reason: no existing file (or unreadable) at [{}]"),
                Bucket.PluginName, Bucket.OutputFilePath);
        }
        else
        {
            auto ExistingLines = TArray<FString>{};
            auto NewLines = TArray<FString>{};
            constexpr auto CullEmpty = false;
            ExistingForCompare.ParseIntoArray(ExistingLines, TEXT("\n"), CullEmpty);
            ContentForCompare.ParseIntoArray(NewLines, TEXT("\n"), CullEmpty);

            const auto CommonLineCount = FMath::Min(ExistingLines.Num(), NewLines.Num());
            auto FirstDiffLine = int32{INDEX_NONE};
            for (auto LineIndex = int32{0}; LineIndex < CommonLineCount; ++LineIndex)
            {
                if (NOT ExistingLines[LineIndex].Equals(NewLines[LineIndex], ESearchCase::CaseSensitive))
                {
                    FirstDiffLine = LineIndex;
                    break;
                }
            }

            if (FirstDiffLine != INDEX_NONE)
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[CkAS ES Params] [{}] rewrite reason: first diff at line {} (old lines: {}, new lines: {})\n    old: [{}]\n    new: [{}]"),
                    Bucket.PluginName, FirstDiffLine + 1, ExistingLines.Num(), NewLines.Num(),
                    ExistingLines[FirstDiffLine], NewLines[FirstDiffLine]);
            }
            else
            {
                ck::angelscriptgenerator::Log(
                    TEXT("[CkAS ES Params] [{}] rewrite reason: line-count change only — old lines: {}, new lines: {}, first extra line: [{}]"),
                    Bucket.PluginName, ExistingLines.Num(), NewLines.Num(),
                    ExistingLines.Num() > NewLines.Num()
                        ? ExistingLines[CommonLineCount]
                        : NewLines[CommonLineCount]);
            }
        }

#if PLATFORM_WINDOWS
        // Match project convention: .as files live as CRLF on Windows disk (checkout converts
        // index-LF → WC-CRLF via core.autocrlf). Without this step, the generator writes LF,
        // disagrees with every other .as file on disk, and git flags the file as modified on
        // every editor startup even though `git diff` is empty.
        Content.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
        Content.ReplaceInline(TEXT("\n"), TEXT("\r\n"));
#endif

        if (FFileHelper::SaveStringToFile(Content, *Bucket.OutputFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            ck::angelscriptgenerator::Log(
                TEXT("[CkAS ES Params] [{}] {} classes, {} properties -> {}"),
                Bucket.PluginName, Bucket.Classes.Num(), BucketProperties, Bucket.OutputFilePath);
        }
        else
        {
            ck::angelscriptgenerator::Error(
                TEXT("[CkAS ES Params] Failed to write to [{}]"),
                Bucket.OutputFilePath);
        }

        TotalClasses += Bucket.Classes.Num();
        TotalProperties += BucketProperties;
    }

    ck::angelscriptgenerator::Log(
        TEXT("[CkAS ES Params] Done — {} plugins, {} classes, {} properties total"),
        Buckets.Num(), TotalClasses, TotalProperties);

#endif // WITH_EDITOR
}

// --------------------------------------------------------------------------------------------------------------------
