#include "CkAngelscriptCompileGuard.h"

#if WITH_EDITOR && WITH_ANGELSCRIPT_CK

#include "CkAngelscriptGenerator_Log.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"

#include <AngelscriptCodeModule.h>
#include <AngelscriptManager.h>
#include <Editor.h>
#include <Modules/ModuleManager.h>
#include <Misc/OutputDevice.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator
{
    namespace ck_angelscript_compile_guard
    {
        class FCk_AngelscriptErrorCapture : public FOutputDevice
        {
        public:
            virtual auto
            Serialize(
                const TCHAR* InMessage,
                ELogVerbosity::Type InVerbosity,
                const FName& InCategory) -> void override
            {
                // AS declares its category without the "Log" prefix
                // (DECLARE_LOG_CATEGORY_EXTERN(Angelscript, ...)), so the FName is NOT "LogAngelscript".
                static const auto AngelscriptCategory = FName{TEXT("Angelscript")};

                if (InCategory != AngelscriptCategory)
                { return; }

                if (InVerbosity > ELogVerbosity::Error)
                { return; }

                // A RUNTIME script exception (FAngelscriptManager::Throw during PIE) logs on this same
                // category — latching it as a compile error made every later PIE start claim "broken/stale
                // script" for a script that compiled clean. Classify by context: initial compile, hot reload
                // (covers preprocessor-stage failures, which never broadcast PreCompile), or CompileModules.
                // Order matters — the static short-circuits keep Get() unreached before the manager exists.
                const auto bCompileContext =
                    NOT FAngelscriptManager::bIsInitialCompileFinished ||
                    FAngelscriptManager::bIsHotReloading ||
                    FAngelscriptManager::Get().bIsCompilingModules;

                if (bCompileContext)
                {
                    // Preprocessor-stage failures never broadcast PreCompile, so the buffer is never
                    // reset and AS re-logs identical lines on every retry — AddUnique keeps the toast clean.
                    _CapturedLines.AddUnique(FString{InMessage});

                    // GetReloadHadErrors() only fires from CompileModules(); preprocessor-stage failures
                    // in PerformHotReload() surface ONLY as this log line, so the flag must be set here.
                    _bHasOutstandingErrors = true;
                }
                else
                {
                    _RuntimeExceptionLines.AddUnique(FString{InMessage});
                }
            }

            auto Get_Lines()  const -> const TArray<FString>& { return _CapturedLines; }
            auto Has_Errors() const -> bool                   { return _bHasOutstandingErrors; }
            auto Get_RuntimeLines() const -> const TArray<FString>& { return _RuntimeExceptionLines; }
            auto Reset_RuntimeLines()     -> void             { _RuntimeExceptionLines.Reset(); }
            auto Reset()            -> void                   { _CapturedLines.Reset(); _bHasOutstandingErrors = false; _RuntimeExceptionLines.Reset(); }

        private:
            TArray<FString> _CapturedLines;
            TArray<FString> _RuntimeExceptionLines;
            bool            _bHasOutstandingErrors = false;
        };

        // Game-thread only (compile delegates + PreBeginPIE all fire there) — no synchronization.
        FCk_AngelscriptErrorCapture _LogCapture;
        FDelegateHandle             _PreCompileHandle;
        FDelegateHandle             _PreBeginPIEHandle;
        bool                        _bInstalled = false;
    }

    auto
        FCk_AngelscriptCompileGuard::
        Install()
        -> void
    {
        if (ck_angelscript_compile_guard::_bInstalled)
        { return; }

        if (ck::IsValid(GLog, ck::IsValid_Policy_NullptrOnly{}))
        {
            GLog->AddOutputDevice(&ck_angelscript_compile_guard::_LogCapture);
        }

        ck_angelscript_compile_guard::_PreCompileHandle = FAngelscriptCodeModule::GetPreCompile().AddLambda([]()
        {
            ck_angelscript_compile_guard::_LogCapture.Reset();
        });

        ck_angelscript_compile_guard::_PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddLambda([](const bool /*bIsSimulating*/)
        {
            if (ck_angelscript_compile_guard::_LogCapture.Has_Errors())
            {
                const auto& Lines = ck_angelscript_compile_guard::_LogCapture.Get_Lines();

                auto Segments = TArray<FCk_TokenizedMessage>{};
                Segments.Reserve(Lines.Num() + 1);
                Segments.Add(FCk_TokenizedMessage{TEXT("AngelScript compile errors detected -- starting PIE with broken/stale script. See diagnostics below:")});
                for (const auto& Line : Lines)
                {
                    Segments.Add(FCk_TokenizedMessage{Line});
                }

                UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage
                (
                    FCk_Utils_EditorOnly_PushNewEditorMessage_Params
                    {
                        TEXT("Angelscript"),
                        FCk_MessageSegments{Segments}
                    }
                    .Set_MessageSeverity(ECk_EditorMessage_Severity::Error)
                    .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::Display)
                    .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::Focus)
                );
            }

            // Runtime exceptions are surfaced ONCE, worded as what they are — the script compiled clean, so
            // claiming "broken/stale script" here sends the user chasing a compile problem that does not exist.
            if (const auto& RuntimeLines = ck_angelscript_compile_guard::_LogCapture.Get_RuntimeLines();
                RuntimeLines.Num() > 0)
            {
                auto Segments = TArray<FCk_TokenizedMessage>{};
                Segments.Reserve(RuntimeLines.Num() + 1);
                Segments.Add(FCk_TokenizedMessage{TEXT("AngelScript RUNTIME exceptions were raised since the last compile (the script itself compiled clean -- this is a gameplay bug, not a stale script):")});
                for (const auto& Line : RuntimeLines)
                {
                    Segments.Add(FCk_TokenizedMessage{Line});
                }

                UCk_Utils_EditorOnly_UE::Request_PushNewEditorMessage
                (
                    FCk_Utils_EditorOnly_PushNewEditorMessage_Params
                    {
                        TEXT("Angelscript"),
                        FCk_MessageSegments{Segments}
                    }
                    .Set_MessageSeverity(ECk_EditorMessage_Severity::Warning)
                    .Set_ToastNotificationDisplayPolicy(ECk_EditorMessage_ToastNotification_DisplayPolicy::Display)
                    .Set_MessageLogDisplayPolicy(ECk_EditorMessage_MessageLog_DisplayPolicy::DoNotFocus)
                );

                ck_angelscript_compile_guard::_LogCapture.Reset_RuntimeLines();
            }
        });

        ck_angelscript_compile_guard::_bInstalled = true;
    }

    auto
        FCk_AngelscriptCompileGuard::
        Uninstall()
        -> void
    {
        if (NOT ck_angelscript_compile_guard::_bInstalled)
        { return; }

        if (FModuleManager::Get().IsModuleLoaded(TEXT("AngelscriptCode")))
        {
            if (ck_angelscript_compile_guard::_PreCompileHandle.IsValid())
            { FAngelscriptCodeModule::GetPreCompile().Remove(ck_angelscript_compile_guard::_PreCompileHandle); }
        }
        ck_angelscript_compile_guard::_PreCompileHandle.Reset();

        if (ck_angelscript_compile_guard::_PreBeginPIEHandle.IsValid())
        {
            FEditorDelegates::PreBeginPIE.Remove(ck_angelscript_compile_guard::_PreBeginPIEHandle);
            ck_angelscript_compile_guard::_PreBeginPIEHandle.Reset();
        }

        if (ck::IsValid(GLog, ck::IsValid_Policy_NullptrOnly{}))
        {
            GLog->RemoveOutputDevice(&ck_angelscript_compile_guard::_LogCapture);
        }
        ck_angelscript_compile_guard::_LogCapture.Reset();
        ck_angelscript_compile_guard::_bInstalled = false;
    }
}

#endif // WITH_EDITOR && WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
