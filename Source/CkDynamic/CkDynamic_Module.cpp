#include "CkDynamic_Module.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Payload/CkPayload.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkDynamic/CkDynamic_Fragment.h"
#include "CkDynamic/CkDynamic_FragmentDisplaySchema.h"
#include "CkDynamic/CkDynamic_ScriptProcessor_Host.h"
#include "CkDynamic/CkDynamic_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

#define LOCTEXT_NAMESPACE "FCkDynamicModule"

void FCkDynamicModule::StartupModule()
{
#if WITH_ANGELSCRIPT_CK
    ck::dynamic::Startup_AngelscriptFragmentDisplaySchemas();
#endif

    ck::FScriptProcessor_Host::Startup();

    // Generic catch-all for replicated dynamic fragments: their payload UScriptStruct is only known at
    // runtime, so no per-type handler can be registered. The dispatcher invokes Apply on the game
    // thread after composition — dynamic fragments need no composition, so write storage + broadcast
    // RepNotify directly and return Applied.
    FCk_PersistenceHandlerRegistry::RegisterFallback(
    {
        .NetApply = [](FCk_Handle& InEntity, const FInstancedStruct& InNew, const TOptional<FInstancedStruct>& /*InOld*/) -> ECk_Persistence_ApplyResult
        {
            const auto* Type = InNew.GetScriptStruct();
            const auto TypeIsValid = ck::IsValid(Type);
            CK_ENSURE_IF_NOT(TypeIsValid,
                TEXT("Dynamic Fragment net hydration received payload without a valid struct type"))
            {}
            if (NOT TypeIsValid)
            { return ECk_Persistence_ApplyResult::Rejected; }

            const auto EntityIsValid = ck::IsValid(InEntity);
            CK_ENSURE_IF_NOT(EntityIsValid,
                TEXT("Dynamic Fragment net hydration received an invalid entity [{}]"), InEntity)
            {}
            if (NOT EntityIsValid)
            { return ECk_Persistence_ApplyResult::Rejected; }

            auto* Storage = UCk_Utils_DynamicFragment_UE::TryAddOrGet_Fragment_TypeUnsafe(InEntity, Type);
            if (Storage == nullptr)
            { return ECk_Persistence_ApplyResult::Rejected; }

            *Storage = InNew;

            auto Info = FCk_DynamicFragment_RepNotifyInfo{};
            Info.ChangedType = const_cast<UScriptStruct*>(Type);

            ck::UUtils_Signal_DynamicFragment_OnRepNotify::Broadcast(InEntity, ck::MakePayload(InEntity, Info));

            return ECk_Persistence_ApplyResult::Applied;
        }
    });
}

void FCkDynamicModule::ShutdownModule()
{
    ck::FScriptProcessor_Host::Shutdown();

#if WITH_ANGELSCRIPT_CK
    ck::dynamic::Shutdown_AngelscriptFragmentDisplaySchemas();
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkDynamicModule, CkDynamic)
