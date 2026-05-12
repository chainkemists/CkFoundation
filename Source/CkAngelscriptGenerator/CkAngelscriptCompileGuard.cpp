#include "CkAngelscriptCompileGuard.h"

#if WITH_EDITOR && WITH_ANGELSCRIPT_CK

#include "CkAngelscriptGenerator_Log.h"

#include "CkCore/EditorOnly/CkEditorOnly_Utils.h"

#include <AngelscriptCodeModule.h>
#include <Editor.h>
#include <Modules/ModuleManager.h>
#include <Misc/OutputDevice.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator
{
    namespace
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
                // NOTE: AngelScript's log category is declared without the "Log" prefix:
                //   DECLARE_LOG_CATEGORY_EXTERN(Angelscript, Log, All)
                // so the FName is "Angelscript", not "LogAngelscript".
                static const auto AngelscriptCategory = FName{TEXT("Angelscript")};

                if (InCategory != AngelscriptCategory)
                { return; }

                if (InVerbosity > ELogVerbosity::Error)
                { return; }

                // Dedupe: AS retries previously-failed files on every save, so when the failure
                // is preprocessor-stage (which never broadcasts PreCompile to reset our buffer),
                // the same error lines re-log on every retry. AddUnique keeps the toast clean.
                _CapturedLines.AddUnique(FString{InMessage});

                // Belt-and-suspenders: the dedicated GetReloadHadErrors() delegate only fires
                // from CompileModules(); preprocessor-stage failures in PerformHotReload() never
                // reach CompileModules and only show up as a UE_LOG(Angelscript, Error, ...) line.
                // Capturing here covers that path too.
                _bHasOutstandingErrors = true;
            }

            auto Get_Lines()  const -> const TArray<FString>& { return _CapturedLines; }
            auto Has_Errors() const -> bool                   { return _bHasOutstandingErrors; }
            auto Reset()            -> void                   { _CapturedLines.Reset(); _bHasOutstandingErrors = false; }

        private:
            TArray<FString> _CapturedLines;
            bool            _bHasOutstandingErrors = false;
        };

        // Module-static state. All access happens on the game thread (compile delegates +
        // PreBeginPIE all fire there), so no synchronization needed.
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
        if (_bInstalled)
        { return; }

        if (ck::IsValid(GLog, ck::IsValid_Policy_NullptrOnly{}))
        {
            GLog->AddOutputDevice(&_LogCapture);
        }

        _PreCompileHandle = FAngelscriptCodeModule::GetPreCompile().AddLambda([]()
        {
            _LogCapture.Reset();
        });

        _PreBeginPIEHandle = FEditorDelegates::PreBeginPIE.AddLambda([](const bool /*bIsSimulating*/)
        {
            if (NOT _LogCapture.Has_Errors())
            { return; }

            const auto& Lines = _LogCapture.Get_Lines();

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
        });

        _bInstalled = true;
    }

    auto
        FCk_AngelscriptCompileGuard::
        Uninstall()
        -> void
    {
        if (NOT _bInstalled)
        { return; }

        if (FModuleManager::Get().IsModuleLoaded(TEXT("AngelscriptCode")))
        {
            if (_PreCompileHandle.IsValid())
            { FAngelscriptCodeModule::GetPreCompile().Remove(_PreCompileHandle); }
        }
        _PreCompileHandle.Reset();

        if (_PreBeginPIEHandle.IsValid())
        {
            FEditorDelegates::PreBeginPIE.Remove(_PreBeginPIEHandle);
            _PreBeginPIEHandle.Reset();
        }

        if (ck::IsValid(GLog, ck::IsValid_Policy_NullptrOnly{}))
        {
            GLog->RemoveOutputDevice(&_LogCapture);
        }
        _LogCapture.Reset();
        _bInstalled = false;
    }
}

#endif // WITH_EDITOR && WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------
