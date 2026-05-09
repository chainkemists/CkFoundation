// Language=angelscript

//============================================================================
// CK ISKM RENDERER — ANGELSCRIPT SANDBOX (Phase O2)
//============================================================================
//
// Compile-time smoke test: verifies the AS binding generator emits compileable
// wrappers for every public Utils method on `utils_iskm_renderer` and
// `utils_iskm_proxy`, and that AS sees every BlueprintType struct/enum
// exported from the module. The test IS the AS compile pass — this function
// is never called at runtime.
//
// Delete after Plan-1 finishes (Phase Q's AutoTests + the gym in Phase P
// exercise the real success paths).
//
//============================================================================

namespace iskm_sandbox
{
    void DemonstrateApi(FCk_Handle InEntity, UCk_IskmRenderer_Data InRendererData)
    {
        // ----- Renderer side -----
        auto Renderer = utils_iskm_renderer::Add(InEntity, InRendererData);
        utils_iskm_renderer::Has(InEntity);

        // ----- Proxy creation -----
        // Use the CK_DEFINE_CONSTRUCTORS-generated 2-arg constructor; CK_PROPERTY
        // backing fields like _Renderer/_SpawnTransform aren't exposed to AS as
        // direct struct members, so go through the constructor (or Set_*).
        auto ProxyParams = FCk_Fragment_IskmProxy_ParamsData(Renderer, FTransform::Identity);
        auto Proxy = utils_iskm_proxy::Add(InEntity, ProxyParams);
        utils_iskm_proxy::Has(InEntity);

        // ----- Animation playback -----
        FCk_Request_IskmProxy_PlayAnimation PlayReq;
        utils_iskm_proxy::Request_PlayAnimation(Proxy, PlayReq);

        FCk_Request_IskmProxy_StopAnimation StopReq;
        utils_iskm_proxy::Request_StopAnimation(Proxy, StopReq);

        utils_iskm_proxy::Request_SetPlayRate(Proxy, 1.5f);

        // ----- Custom data -----
        utils_iskm_proxy::Request_SetCustomDataFloat(Proxy, 0, 0.5f);
        auto CustomVal = utils_iskm_proxy::Get_CustomDataFloat(Proxy, 0);

        // ----- Submeshes -----
        utils_iskm_proxy::Request_AttachSubmesh(Proxy, n"Hat");
        utils_iskm_proxy::Request_DetachSubmesh(Proxy, n"Hat");
        utils_iskm_proxy::Request_DetachAllSubmeshes(Proxy);
        auto NumSubmeshes = utils_iskm_proxy::Get_NumAttachedSubmeshes(Proxy);

        // ----- AnimBP path -----
        TSubclassOf<UAnimInstance> NullClass;
        utils_iskm_proxy::Request_SetAnimInstanceClass(Proxy, NullClass);
        auto Pose = utils_iskm_proxy::Get_PoseSource(Proxy);

        // ----- Montages -----
        FCk_Request_IskmProxy_PlayMontage MontageReq;
        utils_iskm_proxy::Request_PlayMontage(Proxy, MontageReq);

        FCk_Request_IskmProxy_StopMontage StopMontageReq;
        utils_iskm_proxy::Request_StopMontage(Proxy, StopMontageReq);

        // ----- Ragdoll -----
        FCk_Request_IskmProxy_BeginRagdoll RagReq;
        utils_iskm_proxy::Request_BeginRagdoll(Proxy, RagReq);
        utils_iskm_proxy::Request_EndRagdoll(Proxy);

        // ----- Sockets / line trace -----
        auto SocketXf = utils_iskm_proxy::Get_SocketTransform(
            Proxy, n"hand_r", ECk_IskmProxy_TransformSpace::World);

        FCk_IskmProxy_LineTraceParams LtParams;
        FCk_IskmProxy_LineTraceResult LtResult;
        utils_iskm_proxy::LineTrace_Instance(Proxy, LtParams, LtResult);

        // ----- Accessors -----
        auto PlayingAnim = utils_iskm_proxy::Get_PlayingAnimation(Proxy);
        auto PlayTime = utils_iskm_proxy::Get_PlayTime(Proxy);
        auto PlayLength = utils_iskm_proxy::Get_PlayLength(Proxy);
    }
}
