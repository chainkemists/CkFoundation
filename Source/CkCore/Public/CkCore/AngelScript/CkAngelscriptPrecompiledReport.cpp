#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"

#include <HAL/FileManager.h>
#include <Misc/CoreDelegates.h>
#include <Misc/Paths.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptCodeModule.h>
#include <AngelscriptManager.h>

// StaticJITHeader.h is authored for GENERATED translation units: it disables six MSVC warnings
// (4101/4102/4191/4996/4883/4702) and calls PRAGMA_DISABLE_DEPRECATION_WARNINGS with NO matching
// re-enable, plus a clang -Wself-assign suppression with no push. In a generated file that leaks to
// the rest of that file and nothing cares; here it would leak into every OTHER CkCore file sharing
// the unity blob (adaptive unity excludes this file only while it is in the working set).
//
// The push/pop pairs below contain the WARNING state on both toolchains. They cannot contain the
// MACROS it defines (SCRIPT_*, AS_FORCE_LINK, AS_JIT_DEBUG_CALLSTACKS, and whatever the raw as_*.h
// internals declare) - those do leak into the rest of the blob. All are SCRIPT_/AS_/as-prefixed and
// collision-free today; a future collision shows up as a compile error in an unrelated CkCore file.
#ifdef _MSC_VER
#pragma warning(push)
#endif
#ifdef __clang__
#pragma clang diagnostic push
#endif
#include <StaticJIT/StaticJITHeader.h>
PRAGMA_ENABLE_DEPRECATION_WARNINGS
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif

// --------------------------------------------------------------------------------------------------------------------
// Boot-time report on the AngelScript precompiled-script cache and the StaticJIT transpiled code.
//
// The engine computes all of this already and logs a Warning on each rejection path. What it has
// never had is a CONSUMER - FAngelscriptManager::bStaticJITTranspiledCodeLoaded is assigned and read
// by nothing - which left three distinct failures indistinguishable from success in a packaged log:
// a STALE cache (its only validity check is a build-configuration int, so a cache older than the
// scripts loads silently and runs old code), a cache/binary GUID MISMATCH that discards every
// transpiled function, and generated code that PREPROCESSED TO NOTHING because each generated file
// carries the GENERATING exe's own "#if UE_BUILD_<CONFIG>".
//
// One reporter covers the feature's whole state rather than three log lines at three sites, so the
// fourth silent mode is covered by construction. Nothing is cached: every value is read through at
// report time, because a copy of engine state is a mirror that disagrees after the engine changes.
//
// Deliberately NOT covered: the transpiled function COUNT and the cache's own generation GUID. Both
// live on engine-private types that Initialize() clears and deletes before any external module can
// look, so neither is observable from here. The surviving boolean is the "zero or not" detector all
// three failures reduce to, and the cache/corpus pairing record belongs to the build pipeline that
// owns both artifacts, not to a runtime reporter.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_angelscript_precompiled_report
{
#if WITH_ANGELSCRIPT_CK

    // ----------------------------------------------------------------------------------------------------------------
    // MIRROR OF ENGINE POLICY, not of engine state: this reproduces the cache-filename selection in
    // FAngelscriptManager::Initialize (config-suffixed name first, unsuffixed fallback). A mirror of
    // policy drifts silently when the engine renames, which tenet 6 forbids leaving unreconciled.
    // The reconciler is cheap and lives in the output: every probed path is PRINTED, so a drift is
    // visible in the same log line it would otherwise corrupt.
    // ----------------------------------------------------------------------------------------------------------------
    auto Get_CandidateCachePaths() -> TArray<FString>
    {
        const auto ScriptRoot = FAngelscriptManager::GetScriptRootDirectory();

        auto Paths = TArray<FString>{};

#if UE_BUILD_SHIPPING
        Paths.Add(ScriptRoot / TEXT("PrecompiledScript_Shipping.Cache"));
#elif UE_BUILD_TEST
        Paths.Add(ScriptRoot / TEXT("PrecompiledScript_Test.Cache"));
#elif UE_BUILD_DEVELOPMENT
        Paths.Add(ScriptRoot / TEXT("PrecompiledScript_Development.Cache"));
#endif
        Paths.Add(ScriptRoot / TEXT("PrecompiledScript.Cache"));

        return Paths;
    }

    // ----------------------------------------------------------------------------------------------------------------

    auto Report() -> void
    {
        // Expected control flow, not a suppressed problem (tenet 1): the editor never takes the
        // cache path at all - bUsePrecompiledData is hard-false under WITH_EDITOR - so there is
        // nothing to report and a per-boot line would be pure noise.
        if (GIsEditor)
        { return; }

        if (NOT FAngelscriptManager::IsInitialized())
        {
            ck::core::Warning(TEXT("[AsPrecompile] AngelScript is not initialized. "
                "Cannot report script-cache or StaticJIT state for this boot."));
            return;
        }

        // Load-bearing, not defensive: this delegate DOES broadcast on a generation boot (the exit is
        // a soft RequestExitWithStatus honored later in Launch.cpp, not an abort). Every verdict below
        // would be true-but-misleading there, because nothing is registered on a generation boot by
        // construction - it is the run that WRITES the cache, not one that consumes it.
        if (FAngelscriptManager::bGeneratePrecompiledData)
        { return; }

        const auto& Manager = FAngelscriptManager::Get();

        // ---------------------------------------------------------------- cache
        const auto Candidates = Get_CandidateCachePaths();

        auto ProbeSummary  = FString{};
        auto UsedPath      = FString{};
        auto UsedSizeBytes = int64{0};
        auto UsedWrittenAt = FDateTime{};
        auto AnyCacheOnDisk = false;

        for (const auto& Path : Candidates)
        {
            const auto SizeBytes = IFileManager::Get().FileSize(*Path);
            const auto CacheFileExists = SizeBytes >= 0;

            if (NOT ProbeSummary.IsEmpty())
            { ProbeSummary += TEXT("; "); }

            if (CacheFileExists)
            {
                const auto WrittenAt = IFileManager::Get().GetTimeStamp(*Path);

                ProbeSummary += FString::Printf(TEXT("'%s' present (%.1f MB, written %s)"),
                    *Path, static_cast<double>(SizeBytes) / (1024.0 * 1024.0), *WrittenAt.ToString());

                if (NOT AnyCacheOnDisk)
                {
                    UsedPath       = Path;
                    UsedSizeBytes  = SizeBytes;
                    UsedWrittenAt  = WrittenAt;
                }

                AnyCacheOnDisk = true;
            }
            else
            {
                ProbeSummary += FString::Printf(TEXT("'%s' absent"), *Path);
            }
        }

        if (NOT Manager.bUsePrecompiledData)
        {
            ck::core::Display(TEXT("[AsPrecompile] Cache: DISABLED for this run - scripts compiled from SOURCE "
                "(an editor binary launched -game, -as-development-mode, -as-ignore-precompiled-data, or a "
                "commandlet). Probed: [{}]"), ProbeSummary);
        }
        else if (Manager.bUsedPrecompiledDataForPreprocessor && NOT AnyCacheOnDisk)
        {
            // The reconciler firing. We reproduce the engine's cache-filename selection, so "the engine
            // consumed a cache that matches none of the paths we probe" means that policy has drifted.
            // Reporting the drift is right; printing an empty path and a year-1 timestamp is not.
            ck::core::Warning(TEXT("[AsPrecompile] Cache: IN USE, but at a path this build does not know about - "
                "the engine's cache-filename policy has drifted from the one reproduced here. Probed: [{}]"),
                ProbeSummary);
        }
        else if (Manager.bUsedPrecompiledDataForPreprocessor)
        {
            // The write time is the honest staleness signal. The cache carries a generation GUID,
            // but it is regenerated per run and means nothing to a human reading one log; what
            // reveals "this cache predates my script edits" is when it was written.
            ck::core::Display(TEXT("[AsPrecompile] Cache: IN USE - '{}' ({} MB, written {}). "
                "Compare that write time against your script edits: the cache's only validity check is the build "
                "configuration, so a stale cache loads silently and runs old code."),
                UsedPath,
                FString::Printf(TEXT("%.1f"), static_cast<double>(UsedSizeBytes) / (1024.0 * 1024.0)),
                UsedWrittenAt.ToString());
        }
        else if (AnyCacheOnDisk)
        {
            ck::core::Warning(TEXT("[AsPrecompile] Cache: PRESENT BUT NOT USED - scripts compiled from SOURCE. "
                "A cache exists but was not consumed; the engine discards a cache built for a different build "
                "configuration. Probed: [{}]"), ProbeSummary);
        }
        else
        {
            ck::core::Display(TEXT("[AsPrecompile] Cache: NONE - scripts compiled from SOURCE. Probed: [{}]"),
                ProbeSummary);
        }

        // ------------------------------------------------------------- staticjit
        const auto* CompiledInfo = FStaticJITCompiledInfo::Get();
        const auto  TranspiledActive = FAngelscriptManager::bStaticJITTranspiledCodeLoaded;

        if (TranspiledActive)
        {
            ck::core::Display(TEXT("[AsPrecompile] StaticJIT: ACTIVE - this binary carries transpiled C++ (guid {}) "
                "and the loaded cache matched it."),
                CompiledInfo != nullptr ? CompiledInfo->PrecompiledDataGuid.ToString() : FString(TEXT("<unknown>")));
        }
        else if (CompiledInfo == nullptr)
        {
            ck::core::Display(TEXT("[AsPrecompile] StaticJIT: INACTIVE - this binary carries NO transpiled C++. "
                "Expected unless the JIT module was linked in: nothing linked, AS_SKIP_JITTED_CODE set, or the "
                "generated files were compiled into a configuration other than the one that generated them and "
                "preprocessed away. All script runs on the VM."));
        }
        else if (NOT Manager.bUsedPrecompiledDataForPreprocessor)
        {
            // Transpiled code is present but idle simply because no cache was consumed - the flag above
            // is only ever assigned on a boot that loaded one. This is NOT a mismatch, and saying so
            // would be a confident lie on an ordinary -as-development-mode boot of a JIT-carrying build.
            ck::core::Display(TEXT("[AsPrecompile] StaticJIT: INACTIVE - this binary carries transpiled C++ "
                "(guid {}), but no script cache was consumed this boot, so none of it could be used. "
                "See the Cache line above for why. All script runs on the VM."),
                CompiledInfo->PrecompiledDataGuid.ToString());
        }
        else
        {
            // A consumed cache, transpiled C++ in the binary, and nothing registered. THAT combination is
            // a build-pipeline defect wearing a runtime disguise: the package paired a cache and a binary
            // that were not generated together, so every transpiled function silently falls back to the VM
            // while the build reads as successful.
            //
            // State EVIDENCE, not a cause we cannot observe. The engine performed the guid comparison and
            // logged its own verdict; do not assert here something a future engine change could falsify.
            ck::core::Error(TEXT("[AsPrecompile] StaticJIT: INACTIVE although this binary carries transpiled "
                "C++ (guid {}) AND a script cache was consumed. Consistent with the engine discarding all "
                "transpiled code because the two came from different generation runs - see the Angelscript "
                "warning above for the engine's own verdict. Every script function is on the VM."),
                CompiledInfo->PrecompiledDataGuid.ToString());

            // Fires in EVERY configuration, Shipping included: CK_ENSURE is defined unconditionally and
            // every reachable CkBuildConfig branch sets CK_DISABLE_ENSURE_CHECKS=0. That is deliberate -
            // a mispackaged build delivering none of the feature should be loud where it ships, not only
            // where a developer would have noticed anyway.
            CK_TRIGGER_ENSURE(TEXT("[AsPrecompile] Transpiled C++ (guid {}) is compiled into this binary but "
                     "NONE of it is active, against a cache that WAS consumed. The package shipped a cache "
                     "and a binary from different generation runs."),
                CompiledInfo->PrecompiledDataGuid.ToString());
        }
    }

#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Hooked on the ONE delegate that is provably ordered after what we read. AngelScript assigns
    // bStaticJITTranspiledCodeLoaded at AngelscriptManager.cpp:573 and broadcasts
    // GetOnInitialCompileFinished from PostInitialize_GameThread at :629, so the value is settled
    // before we run - on the threaded init path too, where the game thread spin-waits while a worker
    // runs Initialize_AnyThread and IsInitialized() is already true with the flag still unset.
    // OnFEngineLoopInitComplete would ALSO work today, but only by an argument about interleaving
    // rather than a guarantee, and a diagnostic that can read a half-initialized value is exactly the
    // confidently-wrong report this file exists to prevent.
    //
    // The up-front check is not redundant: a module loaded AFTER that delegate has already broadcast
    // (late plugin load, editor hot-reload, DLL reload) would subscribe to something that never fires
    // again and report nothing at all - which is exactly the silent absence this file exists to
    // remove. GIsRunning is the right predicate for "too late to catch it" rather than for "it has
    // broadcast": the engine loop only runs long after AngelScript initialization, so reaching here
    // with it set means everything we read is settled. (bIsInitialCompileFinished is NOT usable for
    // this - it is set inside InitialCompile, which runs BEFORE the flag we report on is assigned.)
    // Same guard as CkIO_Utils' blocking-load registrar.
    // ----------------------------------------------------------------------------------------------------------------
    struct FPrecompiledReportRegistrar
    {
        FPrecompiledReportRegistrar()
        {
#if WITH_ANGELSCRIPT_CK
            if (GEngine != nullptr && GIsRunning)
            {
                Report();
                return;
            }

            FAngelscriptCodeModule::GetOnInitialCompileFinished().AddStatic(&Report);
#endif
        }
    };

    static FPrecompiledReportRegistrar GPrecompiledReportRegistrar;
}
