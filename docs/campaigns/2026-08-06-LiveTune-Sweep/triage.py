"""LiveTune coverage triage over CkFoundation.

For every FCk_Fragment_<X>_ParamsData, find which processors consume the corresponding
ck::FFragment_<X>_Params (a `using` alias of the params-data struct). A params fragment read by a
processor that is NOT a Setup/Teardown pass is read after Add => live-read => ViaReplace candidate.
One consumed ONLY by Setup is baked at Add => needs ViaRequest or ViaRebuild.

Heuristic by design: this SIZES the buckets so the sweep can be planned. The sweep still verifies
each feature individually before registering it.
"""
import os, re, collections

SRC = r"D:/Repositories/CkRepos/CkPlugins_Other/Plugins/CkFoundation/Source"

RE_PROC_DECL = re.compile(r"\b(?:class|struct)\b[^;{]*?\b([FT]Processor_[A-Za-z0-9_]+)")
RE_FRAG = re.compile(r"\bFFragment_([A-Za-z0-9_]+)_Params\b")
RE_PARAMS_TYPE = re.compile(r"\bFCk_Fragment_([A-Za-z0-9_]+)_ParamsData\b")
RE_REGISTERED = re.compile(r"Register_Via(?:Replace|Request|Rebuild)<\s*FCk_Fragment_([A-Za-z0-9_]+)_ParamsData\s*>")

def walk(suffixes):
    for root, _, files in os.walk(SRC):
        for f in files:
            if f.endswith(suffixes):
                yield os.path.join(root, f)

def read(p):
    with open(p, "r", encoding="utf-8", errors="ignore") as fh:
        return fh.read()

# fragment feature -> {processor names that reference it}
frag_procs = collections.defaultdict(set)
for path in walk(("_Processor.h", "_Processor.cpp")):
    cur = None
    for line in read(path).splitlines():
        m = RE_PROC_DECL.search(line)
        if m:
            cur = m.group(1)
        for fm in RE_FRAG.finditer(line):
            if cur:
                frag_procs[fm.group(1)].add(cur)

params_types, registered, frag_any = set(), set(), set()
for path in walk((".h",)):
    body = read(path)
    params_types.update(RE_PARAMS_TYPE.findall(body))
    frag_any.update(RE_FRAG.findall(body))
for path in walk((".cpp",)):
    registered.update(RE_REGISTERED.findall(read(path)))

SETUP = re.compile(r"setup|teardown", re.I)

rows = []
for feat in sorted(params_types):
    if feat in registered:
        rows.append(("REGISTERED", feat, "already opted in"))
        continue
    procs = frag_procs.get(feat, set())
    if not procs:
        # Distinguish "params never retained on the entity" (decomposed at Add - the FloatAttribute
        # shape, which is why it needed ViaRebuild) from "retained but only touched outside a
        # processor" (Utils-only; may still re-apply by Replace + a fixup).
        kind = "C1_DECOMPOSED_AT_ADD" if feat not in frag_any else "C2_UTILS_ONLY"
        why = ("no FFragment_%s_Params exists - params are decomposed at Add"
               if kind.startswith("C1") else
               "FFragment_%s_Params exists but no processor reads it") % feat
        rows.append((kind, feat, why))
        continue
    live = sorted(p for p in procs if not SETUP.search(p))
    if live:
        rows.append(("A_LIVE_READ", feat, ", ".join(live[:3])))
    else:
        rows.append(("B_SETUP_BAKED", feat, ", ".join(sorted(procs))))

out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "triage_out.tsv")
with open(out, "w", encoding="utf-8") as fh:
    fh.write("BUCKET\tFEATURE\tEVIDENCE\n")
    for r in rows:
        fh.write("\t".join(r) + "\n")

counts = collections.Counter(r[0] for r in rows)
print("=== BUCKET COUNTS ===")
for k in ("A_LIVE_READ", "B_SETUP_BAKED", "C1_DECOMPOSED_AT_ADD", "C2_UTILS_ONLY", "REGISTERED"):
    print(f"{counts.get(k,0):4d}  {k}")
print(f"{sum(counts.values()):4d}  TOTAL params-data types")
print(f"\nprocessor-attributed params fragments: {len(frag_procs)}")
print(f"table -> {out}")
