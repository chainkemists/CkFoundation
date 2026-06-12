#include "CkRenderTarget_Fragment.h"

#include "CkEcs/Handle/CkDebugCallstack_Macros.h"

#include "CkEcs/Snapshot/CkSnapshot_FragmentRegistry.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Writer.h"
#include "CkEcs/Snapshot/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------

CK_ECS_DEFINE_CALLSTACK_ANGELSCRIPT_UTILS(CKRENDERTARGET_API, render_target, ck::FFragment_RenderTarget_Requests);

// --------------------------------------------------------------------------------------------------------------------

// Tier-C: the owner-hosted RecordOfRenderTargets connection must round-trip so the restored sync child
// resolves by sync name (UCk_Utils_RenderTarget_UE::TryGet_RenderTarget) after a load — otherwise the
// child is orphaned from the owner's record and the restore re-drive cannot reach it. ck:: type hoisted
// to a file-scope alias because CK_REGISTER_SNAPSHOTABLE token-pastes the type name (mirrors CkInventory).
using FSnap_RecordOfRenderTargets = ck::FFragment_RecordOfRenderTargets;
CK_REGISTER_SNAPSHOTABLE(FSnap_RecordOfRenderTargets);

// --------------------------------------------------------------------------------------------------------------------
