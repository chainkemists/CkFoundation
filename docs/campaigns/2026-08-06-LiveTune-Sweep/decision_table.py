"""Emit the complete per-feature LiveTune decision table.

Every remaining registration needs three facts, and each has already cost this campaign a defect
when guessed. This resolves all three mechanically so the sweep is lookup, not investigation:

  FORM     alias / wrapper  -> whether Register must name T_Fragment (the ~69-feature trap)
  ADD      clean / bakes    -> whether a bare registration is honest, or fields are baked at Add
  READS    which processors consume the params fragment after Setup

Run: python decision_table.py  (writes decision_table.tsv beside this file)
"""
import os, re, collections

SRC = r"D:/Repositories/CkRepos/CkPlugins_Other/Plugins/CkFoundation/Source"
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "decision_table.tsv")

RE_PARAMS = re.compile(r"\bFCk_Fragment_([A-Za-z0-9_]+)_ParamsData\b")
RE_ALIAS = re.compile(r"using\s+FFragment_([A-Za-z0-9_]+)_Params\s*=")
RE_WRAP = re.compile(r"struct\s+[A-Z_]*API\s+FFragment_([A-Za-z0-9_]+)_Params\b")
RE_PROC = re.compile(r"\b(?:class|struct)\b[^;{]*?\b([FT]Processor_[A-Za-z0-9_]+)")
RE_FRAG = re.compile(r"\bFFragment_([A-Za-z0-9_]+)_Params\b")
RE_REG = re.compile(r"Register(?:_ViaRebuild)?<\s*FCk_Fragment_([A-Za-z0-9_]+)_ParamsData")
RE_GET = re.compile(r"InParams\.Get_([A-Za-z0-9_]+)\(\)")
SETUP = re.compile(r"setup|teardown", re.I)


def walk(sfx):
    for root, _, files in os.walk(SRC):
        for f in files:
            if f.endswith(sfx):
                yield os.path.join(root, f)


def read(p):
    with open(p, "r", encoding="utf-8", errors="ignore") as fh:
        return fh.read()


params, alias, wrap, registered = set(), set(), set(), set()
for p in walk((".h",)):
    b = read(p)
    params.update(RE_PARAMS.findall(b))
    alias.update(RE_ALIAS.findall(b))
    wrap.update(RE_WRAP.findall(b))
for p in walk((".cpp",)):
    registered.update(RE_REG.findall(read(p)))

frag_procs = collections.defaultdict(set)
for p in walk(("_Processor.h", "_Processor.cpp")):
    cur = None
    for line in read(p).splitlines():
        m = RE_PROC.search(line)
        if m:
            cur = m.group(1)
        for fm in RE_FRAG.finditer(line):
            if cur:
                frag_procs[fm.group(1)].add(cur)

# Params fields read during Add - the baked-state signal.
add_reads = {}
for p in walk(("_Utils.cpp",)):
    m = re.search(r"Ck([A-Za-z0-9_]+)_Utils\.cpp$", os.path.basename(p))
    if not m:
        continue
    lines = read(p).splitlines(True)
    start = next((i for i, ln in enumerate(lines) if re.match(r"^\s*Add\(\s*$", ln)), None)
    if start is None:
        continue
    body, depth, seen = [], 0, False
    for ln in lines[start:start + 120]:
        body.append(ln)
        depth += ln.count("{") - ln.count("}")
        seen = seen or "{" in ln
        if seen and depth <= 0 and len(body) > 3:
            break
    add_reads[m.group(1)] = sorted(set(RE_GET.findall("".join(body))))

rows = []
for f in sorted(params):
    if f.startswith("Multiple"):
        rows.append((f, "-", "-", "OUT_OF_SCOPE", "Multiple* bulk-add container"))
        continue
    form = "alias" if f in alias else ("wrapper" if f in wrap else "none")
    procs = frag_procs.get(f, set())
    live = sorted(p for p in procs if not SETUP.search(p))
    reads = add_reads.get(f)

    if f in registered:
        status = "REGISTERED"
    elif form == "none":
        status = "VIA_REBUILD"          # params not retained -> decomposed at Add
    elif not live:
        status = "SETUP_BAKED"
    elif reads:
        status = "PER_FIELD"            # bakes params at Add; split retunable vs baked
    elif reads is None:
        status = "MANUAL"               # no Add() found
    else:
        status = "BARE_OK"
    rows.append((f, form, ",".join(live[:2]) or "-", status, ",".join(reads) if reads else "-"))

with open(OUT, "w", encoding="utf-8") as fh:
    fh.write("FEATURE\tFORM\tREAD_BY\tSTATUS\tADD_READS\n")
    for r in rows:
        fh.write("\t".join(r) + "\n")

c = collections.Counter(r[3] for r in rows)
for k, v in c.most_common():
    print(f"{v:4d}  {k}")
print(f"{sum(c.values()):4d}  TOTAL")
print(f"forms: alias={sum(1 for r in rows if r[1]=='alias')} wrapper={sum(1 for r in rows if r[1]=='wrapper')}")
print(f"-> {OUT}")
