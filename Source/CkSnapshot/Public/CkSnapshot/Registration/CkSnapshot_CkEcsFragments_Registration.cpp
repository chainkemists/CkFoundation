#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include "CkSnapshot/Context/CkSnapshot_FragmentRegistry.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Writer.h"
#include "CkSnapshot/Archive/CkSnapshot_Archive_Reader.h"

// --------------------------------------------------------------------------------------------------------------------
// Path #3 registration site. The SerializeSnapshot METHOD lives in CkEcs (FFragment_EntityScript_Current), but the
// CK_REGISTER_SNAPSHOTABLE registration lives here in CkSnapshot so CkEcs never takes a dependency edge on CkSnapshot
// (which would be a cycle, since CkSnapshot depends on CkEcs). CkSnapshot already depends on CkEcs.
//
// ck:: is hoisted to an unqualified alias because CK_REGISTER_SNAPSHOTABLE token-pastes the type name.

using FSnap_EntityScript_Current = ck::FFragment_EntityScript_Current;
CK_REGISTER_SNAPSHOTABLE(FSnap_EntityScript_Current);

// --------------------------------------------------------------------------------------------------------------------
