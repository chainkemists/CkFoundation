"""For each bucket-A feature, dump what its Add() does beyond storing the params fragment.

A feature whose Add ONLY does Add<FFragment_X_Params>(InParams) (+ empty Current/tags) is a clean
Register<T>({}). Anything that READS a params field at Add and bakes it somewhere else - a cadence
tag, a chrono, a handle, an external object - cannot be re-applied by a fragment write alone and
needs .PostApply or a documented bound. That is the Timer/VisibleRange trap.
"""
import os, re, sys

SRC = r"D:/Repositories/CkRepos/CkPlugins_Other/Plugins/CkFoundation/Source"

FEATURES = """2dGridBlocker 2dGridSystem Acceleration AnimPlan AudioDirector AudioTrack AutoReorient
BallisticMotion BulkAccelerationModifier BulkVelocityModifier CameraShake Compass CrowdAgent
DialogEmitter EntityCollection FogOfWar GeometryCollection Homing InteractSource InteractTarget
Interaction Inventory IskmProxy IsmProxy JoltBody JoltCharacter JoltConstraint Marker Minimap
MontagePlayer PathNetwork PathNetworkFollower RaySense RenderTarget ResolverDataBundle ResolverSource
RewindHistory Sensor Sfx ShapeBox ShapeCapsule ShapeCylinder ShapeSphere Tween VatProxy Velocity
VisibleRange VoiceChannel VoiceTalker VoxelNavOccluder VoxelNavPath VoxelNavVolume
WorldSpaceWidget""".split()

def find_utils(feat):
    hits = []
    for root, _, files in os.walk(SRC):
        for f in files:
            if f.endswith("_Utils.cpp") and f.lower().startswith(("ck" + feat).lower()[:len(feat) + 2]):
                if re.fullmatch(rf"Ck{re.escape(feat)}_Utils\.cpp", f, re.I):
                    hits.append(os.path.join(root, f))
    return hits

# Params-field reads at Add are the signal: InParams.Get_X() used for anything other than the
# fragment store itself.
RE_ADD = re.compile(r"^\s*Add\(\s*$")
RE_GET = re.compile(r"InParams\.Get_([A-Za-z0-9_]+)\(\)")

for feat in FEATURES:
    paths = find_utils(feat)
    if not paths:
        print(f"{feat}\tNO_UTILS_CPP\t-")
        continue

    for p in paths:
        with open(p, "r", encoding="utf-8", errors="ignore") as fh:
            lines = fh.readlines()

        # Walk to the first "Add(" signature, then take the function body.
        start = None
        for i, ln in enumerate(lines):
            if RE_ADD.match(ln):
                start = i
                break
        if start is None:
            print(f"{feat}\tNO_ADD_FN\t{os.path.basename(p)}")
            break

        body, depth, seen = [], 0, False
        for ln in lines[start:start + 120]:
            body.append(ln)
            depth += ln.count("{") - ln.count("}")
            if "{" in ln:
                seen = True
            if seen and depth <= 0 and len(body) > 3:
                break

        text = "".join(body)
        gets = sorted(set(RE_GET.findall(text)))
        # Drop the store line itself; what remains is derived state.
        derived = [g for g in gets]
        verdict = "CLEAN" if not derived else "READS_PARAMS_AT_ADD"
        print(f"{feat}\t{verdict}\t{','.join(derived) if derived else '-'}")
        break
