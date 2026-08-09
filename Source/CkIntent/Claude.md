# CkIntent

**Purpose:** Turn one input source's live state into a *sequence*. CkIntent buffers the raw-input layer every
render frame and appends one plain-value row per FIXED logic frame to a fixed-capacity ring: which buttons went
down and up, which are held, what the configured axis pair reads, and what the router did with every event it
delivered — plus two readings the row could not recompute for itself: the eight-way direction of the axis pair
(hysteresis-damped) and the SOCD-cleaned cardinal state. No edge is lost above 60 fps and none is duplicated
below it — see *Why a buffer sits between the router and the record*. Above the record sits the NOTATION — the
compact string a move is authored in, the definitions it parses to (see *The notation*), and the COMPILED SET
those bake into: resolved button identities, arbitration order, and the deferral verdicts a matcher reads (see
*The bake*). Above that sits the MATCHER — a set running on one input layer, scanning the record backwards from
every press that layer was allowed to see, holding the two genuinely ambiguous presses open for as long as the
verdict says and no longer, and latching a phase per move that a consumer polls, subscribes to, and can CLAIM
exclusively — until the latch decays (see *The matcher* and *Two surfaces*). The editor debugger is `CkIntentDebugger` (CkGameplayDebugger plugin); the interactive gyms are CkTests' `Gym_Input_{Fighting,Souls,Debugger}`.

**Depends on:** `CkCore`, `CkEcs`, `CkEcsExt`, `CkInput`, `CkLog`, `CkSettings` — plus engine `InputCore` and
`GameplayTags`.
**Used by:** nothing in-repo yet; `CkTests` (`Script/CkInput/CkAutoTest_Intent_*.as`) exercises it.

**The dependency on CkInput is one-way and must stay that way.** CkInput knows nothing about this module: the
record is derived from what the raw layer already publishes, so a project can use the whole input stack without
CkIntent and nothing in CkInput has to grow a hook for it.

---

## Key API

### `UCk_Utils_IntentSampler_UE` — `CkIntentSampler_Utils.h`

Handle: `FCk_Handle_IntentSampler`. Composed onto the input-source entity with `Add`; there is no `Create`,
because a sampler on a child entity would have no routed events to read.

| Group | Functions |
|---|---|
| Compose | `Add`, `Has` (C++ only), `Cast`/`CastChecked` (`DoCast`/`DoCastChecked` in BP/AS), `Get_InvalidHandle` |
| Read | `Get_FrameCount`, `Get_LatestFrame`, `TryGet_FrameAtOffset` |

`FCk_Fragment_IntentSampler_ParamsData` is (`_RingCapacity` default 120, `_AxisKeyX` default
`EKeys::Gamepad_LeftX`, `_AxisKeyY` default `EKeys::Gamepad_LeftY`, `_OctantNeutralRadius` default `0.25`,
`_OctantHysteresisMarginDegrees` default `5.0`, `_SocdQuad` default unnamed, `_SocdPolicy` default `Neutral`).
Capacity is the essential constructor argument; everything else has fluent setters.

**There are no requests.** A sampler is pure derivation — it reads state other features own and writes only its
own ring — so nothing about it is deferred and there is nothing for a caller to enqueue. `Add` is the whole
mutating surface, and it takes effect immediately because composing a fragment is not a mutation of anything
already running.

### `FCk_Intent_FrameRecord` — the row

`_FrameIndex` (monotonic from 0, never reset), `_Pressed` / `_Released` / `_Held` (`FCk_Input_ButtonId` arrays),
`_ConditionedAxisX` / `_ConditionedAxisY`, `_RoutedEvents` (`FCk_InputLayer_RoutedEvent` — the CkInput row
carrying each event and its delivery outcome), `_Octant` (`ECk_Intent_Octant`), and `_CleanedHorizontal` /
`_CleanedVertical` (`ECk_Intent_CleanedAxis`).

Rows are plain values with no live handles of their own, which is what lets a consumer copy one out of the ring
and still read it after everything it describes has moved on. It is also what keeps the rollback option open for
free: a record of values can be replayed, a record of references cannot.

The last three fields are DERIVED, and they are on the row rather than left to consumers for one reason: both
readings depend on state that only exists BETWEEN rows — which octant the last boundary was cleared into, and
the order the held cardinals went down. A consumer deriving them from a row's raw values alone would disagree
with its neighbour, and neither could be right. Deriving once, where the state lives, is what makes two readers
of the same row agree.

### `UCk_Utils_IntentGrammar_UE` — `CkIntentGrammar_Utils.h`

No handle, no entity, no fragments: pure data and free functions, every one `BlueprintPure` and reaching all
three environments (`utils_intent_grammar::` in AngelScript).

| Group | Functions |
|---|---|
| Author | `Parse` (notation string → definition or reason), `Get_TerminalStep` |
| Compile | `Bake` (definitions + name→ButtonId rows + chord window → compiled set or reason) |
| Read a set | `TryGet_ResolutionRow`, `Get_DeferralVerdict` |

### `UCk_Utils_IntentMatcher_UE` — `CkIntentMatcher_Utils.h`

Handle: `FCk_Handle_IntentMatcher`. Composed onto an INPUT LAYER with `Add`; there is no `Create`, because a
matcher off a layer would have no arbitration to answer to.

| Group | Functions |
|---|---|
| Compose | `Add`, `Has` (C++ only), `Cast`/`CastChecked` (`DoCast`/`DoCastChecked` in BP/AS), `Get_InvalidHandle` |
| Poll | `Get_IntentPhase`, `Get_IntentPhase_ByName`, `TryGet_CompletionFrame`, `TryGet_CompletionFrame_ByName` |
| Claim | `Get_IsClaimed`, `Get_IsClaimed_ByName`, `TryGet_ClaimedBy`, `TryGet_ClaimedBy_ByName`, `Request_Claim` |
| Inspect | `Get_HasActiveSet`, `Get_ActiveIntentCount`, `Get_RegisteredCaptureKeys` |
| Diagnose | `Get_ScanDiagnostics`, `Get_ScanDiagnosticsEnabled` (see *Scan diagnostics*) |
| Signals | `BindTo_/UnbindFrom_OnIntentPhaseChanged`, `BindTo_/UnbindFrom_OnIntentCompleted` |
| Requests | `Request_SwapSet` (deferred), `Request_Claim` (**immediate** — see *The claim*) |

`FCk_Fragment_IntentMatcher_ParamsData` carries two fields: `_CaptureBehavior` (default `Consume`, the essential
constructor argument) and `_LatchDecayFrames` (default 20, fluent setter, rejected at composition if not
positive). There is deliberately **no input-source field**: the layer already names its source, and a second
spelling is one that can disagree with the first.

---

Parse and Bake sit together because they are one pipeline — notation in, activatable set out — and splitting them
would split the AngelScript namespace for what an author experiences as one surface. The interesting part is how
they divide the validation: **Parse validates one string in isolation; Bake validates everything only a whole set
can answer.**

`FCk_Intent_ParseResult` is Parse's return: `_Outcome` (`ECk_SucceededFailed`) is the mode, `_Error`
(`ECk_Intent_ParseError`) its value, `_Definition` the payload, `_ErrorToken` the offending token verbatim. A
rejected result carries an EMPTY definition — a notation is accepted whole or not at all. `FCk_Intent_BakeResult`
is Bake's, in the same shape, with an empty set and a small set of context fields naming what to fix.

**Malformed input is a RESULT on both, not an ensure.** A typo in an authored move is the expected failure of
these functions, not a violated precondition, so they return a reason the way `FCk_2dGridPlacement_Result` does
rather than firing a diagnostic. Rejections log at `Verbose`.

---

## Composition — what the record says depends on what the source carries

The sampler reads whatever the source happens to have and never requires anything beyond the source itself.

| Composed on the source | Effect on the row |
|---|---|
| `InputButtonMap` | `_Pressed` / `_Released` / `_Held` carry `ButtonId`s |
| *no* `InputButtonMap` | those three are **EMPTY** |
| `InputBias` | axes carry the CONDITIONED values (deadzone, curve, sensitivity, inversion) |
| *no* `InputBias` | axes carry the raw values — identity, which is what "no conditioning" means |
| `InputButtonMap` **and** a named `_SocdQuad` | the cleaned pair reads the quad's held state under the declared policy |
| unnamed `_SocdQuad`, **or** buttons the map never mints | both cleaned slots stay **Neutral** and no SOCD work runs |

**Why no ButtonMap means empty rather than synthesised identities:** a `ButtonId` is minted by exactly one
authority, and the map's Physical tier already exists to give raw keys an identity. A sampler that invented its
own would be a second minting authority whose output nothing downstream could tell apart from a registered one.
Nothing is lost — `_RoutedEvents` still carries every raw key — only unnamed. A project that wants raw keys in
button space declares them as Physical buttons on the map.

**Key → button is one-to-many**, so a key that several mappings share contributes EVERY holder to the row.

---

## Cadence — one row per logic frame, bounded

The sampler processor declares `TickRate = ck::time::Hz(60)`, so a recorded pattern means the same thing on
every machine: a two-frame tap is two rows whether the renderer is at 30 fps or 240. Catch-up stays on the
default `ReplayMissedTicks` — collapsing a hitch into one row would lose every press inside it — and is bounded
by `MaxReplayedTicks = 4`, the shared-base clamp trait ([`CkEcs/CkProcessor.h`](../CkEcs/Public/CkEcs/Processor/CkProcessor.h)).
Four frames still lets an ordinary 30 Hz frame replay both of its logic steps, while a stall cannot deliver a
quarter-second of history in one go. Whole intervals past the bound are dropped, not deferred.

### Why a buffer sits between the router and the record

A fixed cadence over a per-render-frame surface does not compose on its own, and it fails in **both**
directions. The router's retention is cleared at the top of every Route pass: above 60 fps the sampler skips
render frames outright and their events are gone before it ever looks, and below 60 fps its replayed catch-up
ticks re-read the *same* retention and record one press several times.

So the module is two processors, not one. `FProcessor_IntentSampler_Collect` is **unrated** — it runs every
render frame and appends that frame's retention rows to `FFragment_IntentSampler_PendingEvents`.
`FProcessor_IntentSampler_Sample` is the rated one: each logic tick it claims the WHOLE buffer and clears it.

That single pair of choices settles both failures:

| Frame rate | Result |
|---|---|
| **> 60 fps** | Nothing is lost. Events buffered on skipped render frames land in the next logic frame's row. |
| **< 60 fps (catch-up replay)** | Nothing is duplicated. The first replayed tick empties the buffer; the rest of the burst finds it empty and writes rows with no edges. |

**Hitch attribution — the honest cost.** Every event claimed by a logic tick is attributed to THAT tick's row.
Under a hitch, several render frames' worth of input therefore collapses onto one frame index: the record
still contains every edge, in order, but the sub-hitch spacing between them is gone. This is not a bug to
route around — the machine genuinely did not observe those frames separately — and it is bounded by
`MaxReplayedTicks`. A consumer that needs true sub-frame timing needs the timestamps the raw layer carries,
not the record.

---

## The ring

Fixed capacity, overwrite-oldest, count stable at capacity once full. Rows live in one flat array written
circularly rather than a queue that shifts, because a row's storage slot is meaningless to consumers: they
address rows by **offset from the latest** (0 = newest), which is the only addressing that names the same frame
regardless of when it is asked.

`TryGet_FrameAtOffset` answers an out-of-range offset — negative, past the count, or anything at all on an
empty ring — with a row whose `_FrameIndex` is negative. A real row always carries a real index, so the two can
never be confused and there is no separate success flag to forget to check. `Get_LatestFrame` is offset 0 and
answers the same way before the first sampling pass has run.

---

## Direction — the octant, and why it sticks

`ECk_Intent_Octant` is `{ Neutral, E, NE, N, NW, W, SW, S, SE }`, and **the order is the angle order**: `E` is the
axis pair's positive X, and every following value is another 45° counter-clockwise. That makes the whole mapping
arithmetic — an octant's centre is `(value - 1) * 45`, and the octant for an angle is `round(angle / 45)` put back
through the same `+1` — with nothing to look up and no table to keep in step with the enum. `Neutral` takes slot
zero, which is what the `-1` pays for and what makes a default-constructed row read as *no direction* rather than
as East. **`N` is the pair's positive Y, not the world's north**: up on a stick, down on a screen-space pair.

Two parameters shape the reading, and the defaults are a proposal, not a measurement — the hardware spike that
would tune them has not run.

| Parameter | Default | Why that number |
|---|---|---|
| `_OctantNeutralRadius` | `0.25` | Below this magnitude the row reads `Neutral`. It is about direction CONFIDENCE, not noise: a stick a hair off centre has a perfectly real magnitude and a wildly unstable angle. `0.25` is within a hair of XInput's stock left-stick deadzone (`7849/32767 ≈ 0.24`), which is the radius players' hands are already calibrated to. Rejected outside `[0, 1)` — at 1 every direction would read neutral. |
| `_OctantHysteresisMarginDegrees` | `5.0` | How far past a boundary a NEW octant must be entered. Five degrees is ~11% of an octant: wide enough to swallow the two-to-three degrees of jitter a stick resting on a diagonal produces, narrow enough that a deliberate eighth-turn does not feel sticky. Rejected outside `[0, 22.5)` — at half an octant the hold band reaches past the neighbour's centre and the neighbour could never be entered at all. |

**The rule is asymmetric, and that asymmetry IS the mechanism.** Each frame the sampler computes the candidate
octant from the angle, then keeps the PREVIOUS one unless the angle has left the previous octant's *hold band* —
its own 22.5° half-width plus the margin past each boundary. Staying costs nothing; entering costs the margin. A
symmetric rule is exactly the thing that flickers when a value sits on a boundary.

```
angle 20°  (prev: none)  -> E    candidate taken; nothing to hold
angle 26°  (prev: E)     -> E    26 <= 22.5 + 5, still inside E's hold band
angle 30°  (prev: E)     -> NE   30 >  22.5 + 5, the boundary was cleared by more than the margin
angle 21°  (prev: NE)    -> NE   |21 - 45| = 24 <= 27.5 — inside E's territory, still inside NE's hold band
angle 5°   (prev: NE)    -> E    |5 - 45|  = 40 >  27.5
```

**Falling under the neutral radius RESETS the memory.** Hysteresis exists to stop a flicker *inside one gesture*;
a stick that came back to centre has ended the gesture, and carrying the memory through would let it bias the
direction the next one is read as. So `Neutral` is written to the row *and* to the remembered octant, and the
first reading after the stick leaves centre is the plain candidate.

## SOCD — cleaning an opposing pair

Simultaneous Opposite Cardinal Direction cleaning answers the question a keyboard can ask and a gated stick
cannot: what does left-AND-right mean? It runs over `_SocdQuad`, four `FCk_Input_ButtonId`s (`_Up`, `_Down`,
`_Left`, `_Right`), and writes `_CleanedHorizontal` / `_CleanedVertical` onto the row.

**The quad is all four or none, rejected at `Add` otherwise, atomically** — cleaning resolves opposing PAIRS, so
three buttons describe an axis with one end and there is no reading to give for it. A repeated button is rejected
the same way, for the same reason the axis pair rejects one key named twice. A button whose name is `None` is the
unset state, mirroring CkInput's own convention that an invalid `FKey` is the real unset state of an association:
a named button always carries a real name, so no separate flag can go stale against the value.

| `_SocdPolicy` | Both ends held reads | |
|---|---|---|
| `Neutral` *(default)* | `Neutral` | The fighting-game tournament standard, and the only policy that cannot produce a reading the hardware it emulates could not give. |
| `LastInputPriority` | the more recently PRESSED end | The "clean" behaviour most modern pads ship; a re-press while the opposite is down counts as the latest input and moves that end to the front of the order. |
| `FirstInputPriority` | the earlier-pressed end | The opposite ruling — the second press is ignored until the first is released. |

`ECk_Intent_CleanedAxis` is `{ Neutral, Positive, Negative }` — one enum for both slots, not a `Left/Right` and an
`Up/Down` pair. The row already expresses direction as the SIGN of an axis, so `Positive` on the horizontal slot
means the direction `_ConditionedAxisX` reports positive, and the policy that resolves an opposing pair is
written once instead of twice under two spellings of the same three states.

**Press order comes from the CLAIMED events, in arrival order** — not from the row's `_Pressed` set, which is
unordered and therefore cannot say which of two presses the same logic frame claimed came first. The sampler
keeps a small oldest-first list of the cardinals currently down; a press moves its button to the end, and a
button that left the held set leaves the list. Two consequences worth knowing: under a hitch, several render
frames of input collapse onto one row, so two presses the player made milliseconds apart are ordered by the
router's arrival order and nothing finer (the same honest cost *Hitch attribution* already names); and a pair
whose presses PREDATE the sampler has no order to compare, so the priority policies answer `Neutral` rather than
inventing a winner.

**SOCD needs a button map.** The quad names `ButtonId`s, and a source with no `InputButtonMap` has an empty held
set — so a quad on such a source reads `Neutral`/`Neutral` forever. That is not an error and is not ensured
against: the map is opt-in and may be composed after the sampler, and a quad naming buttons the map has simply
not minted yet is a normal startup state, not a misconfiguration.

---

## The notation — how a move is written

This is the authoring reference: everything a designer needs to write a move, and every way one can be rejected.
A move is a STRING, parsed by `UCk_Utils_IntentGrammar_UE::Parse`. The string is the surface on purpose — the
genre already writes moves this way, and a reader takes in a whole move at a glance instead of unfolding a nested
struct. Nothing else produces an `FCk_Intent_Definition`: there is one parser, and the C++ tests, the Blueprint
and AngelScript paths and the bake's fixture all enter through it.

```
236+LP w=200 lenient
└─┬─┘ └┬┘ └──┬───┘
  │    │     └── trailing modifiers, any order
  │    └──────── the + binds to the run's LAST digit
  └───────────── a digit RUN — one step per digit
```

### Directions — the numpad

A direction is a numpad digit, read off the keypad's own face. The mapping is positional, so there is no table to
keep in step with anything:

| Digit | Octant | Digit | Octant | Digit | Octant |
|---|---|---|---|---|---|
| `7` | `NW` | `8` | `N` | `9` | `NE` |
| `4` | `W`  | `5` | `Neutral` | `6` | `E` |
| `1` | `SW` | `2` | `S` | `3` | `SE` |

`ECk_Intent_Octant`'s order IS the angle order (`E` is positive X, counter-clockwise from there), and `N` is the
axis pair's positive Y — up on a stick. `5` is the ABSENCE of a direction: a legitimate step (returning the stick
to centre is a thing a motion asks for), never a chord member. `0` is not a direction and there is no `0` step.

### Steps and sequences

- **A run of digits is a SEQUENCE, one step per digit.** `236` is three steps — down, down-forward, forward.
- **Whitespace separates steps.** `2 8 LP` is three steps. Runs of spaces and tabs, leading and trailing, are all
  the same separator; whitespace never changes what a notation means.
- **A button name is a step.** `LP`, `Attack_Heavy`. Charset is `[A-Za-z0-9_]`, and a name may not start with a
  digit — a token that starts with a digit is a direction run, which is the whole of that rule.
- **The LAST step is the TERMINAL** — the input that completes the move, and what the matcher's backward scan
  anchors on. It is a property of the list, so a definition cannot carry a terminal that disagrees with its steps.

### Chords — `+`

Atoms joined by `+` are ONE step, all required at the same moment: `6+LP`, `LP+MP`, `236+LP`.

- **`+` binds to the run's LAST digit.** `236+LP` is three steps — `2`, `3`, then a chord of `6` and `LP` — not
  one four-atom step. Anything else would ask the player to hit three directions and a button simultaneously.
- **The rule is uniform over position.** A digit run anywhere in a chord token contributes its leading digits as
  steps BEFORE the chord and its final digit to the chord: `LP+236` is `2`, `3`, then a chord of `LP` and `6`.
- **Atoms keep the order they were written in.** `6+LP` and `LP+6` are the same step semantically but compare
  differently; the parser never reorders, so what you read back is what you wrote.
- **Button names are NOT resolved here.** `LP` stays a name until the bake, which is the only place a
  name → ButtonId table exists. A name this project has not declared is a BAKE rejection, not a parse one.

### Modifiers — trailing, order-free

| Modifier | Means |
|---|---|
| `w=<frames>` | the whole-sequence window, in LOGIC FRAMES at the sampler's 60 Hz cadence |
| `hold=<frames>` | the terminal must be held this many frames |
| `lenient` | the matcher may skip transient unmatched directions BETWEEN steps |

- **Frames, never milliseconds.** The record they will be matched against is indexed in frames; a millisecond
  budget would mean a different number of rows on every machine, which is exactly the determinism this module
  exists to buy.
- **Values must be positive integers.** `0`, negatives and non-numerics are rejected, which is what makes a stored
  zero unambiguously mean *not declared* — there is no second flag that could disagree with the value.
- **Order-free among themselves, but strictly TRAILING.** Once a modifier appears, every later token must be one.
  A step after a modifier is either a step written in the wrong place or a misspelled modifier, and guessing which
  would silently reorder the move.
- **Keywords match case-insensitively** (`W=200`, `LENIENT`). That costs a button the right to be named after a
  modifier, deliberately: with case-sensitive keywords, `Lenient` would read as a button token and the modifier
  the designer wrote would vanish with no complaint.

### What the parser does NOT check

Parse-time validation is about the NOTATION and nothing else. Whether `LP` is a declared button, whether two
moves share a name, whether two priorities tie on a shared terminal — none of those can be answered from one
string. They are the bake's, which is the only place the whole set is visible at once. The name, priority and tag
handed to `Parse` are carried through untouched; an unnamed definition is a real problem and a bake-time one.

### Rejections

Every one is distinct, so a message can name the fix and a test can prove it rejected for the reason under test.
The rejected result carries an empty definition and the offending token.

| `ECk_Intent_ParseError` | Fires on | Example |
|---|---|---|
| `EmptyNotation` | an empty or whitespace-only string | `""` |
| `NoSteps` | modifiers and nothing else | `"w=200 lenient"` |
| `UnknownModifier` | a token with `=` whose key is neither `w` nor `hold` | `"236 window=200"` |
| `DuplicateModifier` | the same modifier declared twice | `"236 w=200 w=300"` |
| `ModifierNotTrailing` | a step token after a modifier | `"2 w=200 6"` |
| `MalformedWindow` | `w=` with a non-positive-integer value | `"236 w=0"`, `"236 w=-5"`, `"236 w=abc"` |
| `MalformedHold` | `hold=` under the same rules | `"236 hold=0"` |
| `InvalidDirectionDigit` | a digit-leading token carrying anything but `1`-`9` | `"0"`, `"206"`, `"2a"` |
| `InvalidButtonName` | a button token outside `[A-Za-z0-9_]` | `"L-P"`, `"LP!"` |
| `EmptyChordAtom` | a chord with a missing atom | `"6+"`, `"+LP"`, `"6++LP"` |
| `ChordDuplicateAtom` | the same atom twice in one chord (names compare case-insensitively, as `FName` does) | `"LP+LP"`, `"LP+lp"`, `"6+6"` |
| `ChordTwoDirections` | two directions in one chord — one stick, one octant | `"6+4"`, `"236+4+LP"` |
| `ChordNeutralDirection` | neutral inside a chord — it is the absence of a direction | `"5+LP"`, `"25+LP"` |

Duplicates answer before the two-directions rule: `6+6` is one atom typed twice, and saying so names the fix,
where "a chord cannot hold two directions" would send the author hunting for a second direction they never wrote.

### The definition

`FCk_Intent_Definition` carries `_Name`, `_IntentTag`, `_Steps`, `_WindowFrames`, `_HoldFrames`, `_Priority` and
`_Lenience`. A step is `FCk_Intent_Step` — a set of `FCk_Intent_Atom`, each a direction or a button name — and a
chord is simply a step with more than one atom, not a second shape a consumer has to branch on.

**Definitions are produced ONLY by the parser.** There are no setters, no public field constructor, and no
`CK_DEFINE_CONSTRUCTORS`: the one constructor that can populate an instance is private and
`UCk_Utils_IntentGrammar_UE` is its only friend. Everyone else can make exactly one definition — the default,
empty one — which is the value a rejected parse carries. A definition that exists and is non-empty therefore came
through the grammar, and there is no second dialect a move could have been written in.

**The tag is carried, not minted.** The `Intent.*` namespace question is still open; nothing here registers or
validates a tag.

---

## The bake — from a pile of moves to a set a character can use

`Bake(definitions, name→ButtonId rows, chord window)` answers with an `FCk_Intent_CompiledSet` or the reason
there isn't one. It is a pure function over values: **no world, no entity, no composed ButtonMap.** A caller that
has a map builds rows from it; a cook step, a validation commandlet and a test supply rows they made up, and the
bake cannot tell them apart.

What it produces:

| Part | What it is |
|---|---|
| `_Intents` | the definitions with every button atom resolved to an `FCk_Input_ButtonId` |
| `_ResolutionTable` | per TERMINAL button, the intents a press of it can complete, most-dominant first |
| `_Deferrals` | only the buttons that must not be acted on immediately, and by how many frames |
| `_ChordWindowFrames` | the window inside which two atoms of a chord count as simultaneous |

**A set is values all the way down.** No handles, no pointers, no back-reference to the definitions or the map it
was built from — which is what makes activating one an assignment rather than a rebuild. A state machine can swap
a set in mid-fight and nothing re-resolves, re-sorts or re-validates.

**It is baked once and never edited.** Every table is derived from every intent: add one move and the ordering,
the verdicts and the tie check can all change. There is no coherent "add an intent" — only a rebake.

### Resolution — a press to a shortlist

A press of a button starts a backward scan for every intent whose TERMINAL contains that button, in descending
priority order. A row's entries are indices into `_Intents` rather than copies of names and priorities, because a
row carrying its own copy would be a second place the truth lived.

- **Only terminals are ever visited.** A button appearing mid-sequence gets no row at all — see below.
- **A chord terminal contributes EVERY button in it**, because a press of any of them can begin completing it.
- **An intent whose terminal is a pure direction step contributes no row**: a press cannot complete it. That move
  needs a direction-driven trigger, which is the matcher's to provide, and nothing rejects it here.
- `TryGet_ResolutionRow` answers an EMPTY row for a button no intent terminates on — indices are what a real row
  carries, so there is no found-flag to forget.

**Priorities must be a strict total order per terminal.** Two intents sharing a terminal at the same priority are
rejected, naming both and the button: which one a press resolves to would otherwise be a function of iteration
accident. Equal priorities on DIFFERENT terminals are fine — a tie is only a defect where arbitration would
actually have to choose. Because the tie check runs before the sort, the resulting order is identical on every
machine and every run.

### Deferral — the forward-ambiguity law, as data

**A press is acted on immediately unless waiting is the only way to be right.** The set stores a verdict only for
the buttons that must wait; every other button — including one the set has never heard of — answers a
default-constructed verdict that defers for nothing. That is not an optimisation. It is what makes the law below
impossible to break by accident: no deferral is what happens when nothing wrote a row, so it cannot be forgotten,
mis-computed or regressed by a later change.

**Sharing a terminal NEVER defers.** Two intents ending on the same button — `236+LP` and a longer sequence
ending in `LP` — are decided by what the record already contains *behind* the press. The prefix either happened
or it did not; there is nothing in the future to wait for, so a suffix-sharing button resolves on the press frame.
This is the property the whole design exists to protect, and it is the reason deferral is opt-in rather than
opt-out.

There are exactly two ambiguities that DO reach forward, and both require a rival — with one candidate there is
no other intent a press could have meant, so nothing is ambiguous no matter how the move is shaped:

| Cause | Fires when | Frames |
|---|---|---|
| `_HoldSiblingFrames` | two or more intents share the terminal and at least one declares `hold=` | the longest `hold=` among them |
| `_ChordMemberFrames` | two or more intents share the terminal and at least one's terminal needs a SECOND BUTTON | `_ChordWindowFrames` |

**Only a second BUTTON counts as a chord ambiguity — a direction in a chord does not.** `6+LP` completes or fails
on the `LP` press: the direction is state the frame record already reports on that very frame, so nothing is in
flight and waiting would buy latency for nothing. Two buttons are the case where the partner press may still be
arriving.

Both causes are independent and may both apply; the verdict carries both, and **how to combine them is the
matcher's call**, deliberately not answered by the bake. Zero on a cause means that cause does not apply — the
same not-declared convention `w=` and `hold=` use, and unambiguous for the same reason.

### `ChordWindowFrames`

A bake PARAMETER, uniform across the set, default 3 logic frames (~50 ms at 60 Hz — roughly the window a player
cannot beat on purpose). It is not notation: the grammar has no chord-window syntax and is deliberately kept from
growing one, since per-move chord timing is the kind of knob that turns a readable notation into a config
language. *Revisit if a game genuinely needs per-move chord timing — that is the point at which it becomes a
modifier.* Zero or negative is rejected.

### Rejections

Atomic: any one of these yields an EMPTY set, never the moves that happened to compile. A character silently
missing one special is a bug that ships; a set that refuses to compile is a bug that gets fixed.

| `ECk_Intent_BakeError` | Fires on | Names |
|---|---|---|
| `NoDefinitions` | an empty definition array | — |
| `NonPositiveChordWindow` | a chord window of zero or less | — |
| `UnnamedIntent` | a definition with no name (the only definition anyone outside the parser can make) | — |
| `DuplicateIntentName` | two definitions claiming one name | the contested name |
| `UnknownButtonName` | a button atom no supplied row maps | the intent AND the button name |
| `ConflictingButtonRow` | two rows answering one name with DIFFERENT identities | the button name AND both identities |
| `PriorityTieOnSharedTerminal` | two intents tied on a shared terminal | both intents AND the button |

`UnknownButtonName` keeps the button as a NAME rather than a `ButtonId`: minting an identity to describe a button
that has none would be inventing the very thing the rejection is about.

**A repeated row is only a defect when it disagrees.** The same name against the SAME identity is idempotent and
accepted unchanged — a caller stitching rows from two overlapping sources should not have to de-duplicate first.
Two DIFFERENT identities are two answers to one question: taking either would silently discard a declaration
somebody wrote down, so the whole set refuses. The check runs before a single atom is resolved, which is what
lets the resolver take the first matching row without that being a policy.

## The matcher — a set, a layer, and a backward scan

A matcher is a compiled set running on ONE input layer. That is the whole arbitration story, and it is the reason
this feature is composed rather than global: a set only ever matches the events VISIBLE to its layer, so a modal
that consumes above it silences every move underneath without either side knowing the other exists, and two layers
each carrying their own set (the player's moves, the vehicle's) neither see nor cancel each other. Entering the
vehicle is pushing a layer; leaving it is destroying one.

**No API returns intent state without the layer in the lookup path.** There is no global "did the player just
quarter-circle" and there will not be one — the question is meaningless without saying where it was asked. Every
read takes an `FCk_Handle_IntentMatcher`, which is a handle to a layer, and a consumer holds the one it composed
or was given exactly as it holds every other feature handle in this codebase.

State is per-definition ROWS in one stable fragment, never an entity per activation: pools here are tombstone-mode,
so churning fragment types as a state machine swaps move sets mid-fight is the expensive shape, and the argument
for an O(1) swap is that activating a set touches one value.

### Delivery visibility — the predicate, written once

An event is visible to layer L exactly when nothing ABOVE L ended its walk. Derived per routed-event row:

| Outcome | Visible | Why |
|---|---|---|
| `PassedThrough` | **yes** | Nothing ended the walk, so every layer on the way down was offered it — including this one. |
| `DroppedNoOwner` | no | A release whose press owner is gone. It never walked at all. |
| `ConsumedByLayer`, consumer **is** this layer | **yes** | We are the one that ended it. |
| `ConsumedByLayer`, consumer priority **≤** ours | **yes** | The walk reached us first and continued past us. |
| `ConsumedByLayer`, consumer priority **>** ours | no | It stopped above us. |
| `ConsumedByLayer`, consumer **destroyed since** | no | An unrankable consumer cannot be proven to be below us, and the row says something ended the walk that was not us. |

`PassThrough` deliveries are visible by construction — they never end a walk, so they land in the first row of the
table. **Only press EDGES are filtered this way.** Held-ness carries no delivery outcome of its own (the record
derives `_Held` from every claimed event regardless of who received it), so it is taken at face value.

The predicate bridges button space to key space through the source's button map, because the row names BUTTONS
while the delivery outcomes name KEYS. That makes the answer a statement about the button's CURRENT resolution — a
one-frame skew across a rebind, and the alternative would be stamping a key onto every edge the record already
describes in button space.

### The backward scan

Backwards is the design, not an optimisation. A forward matcher has to keep one partial match per candidate alive
across frames and decide when to abandon each; scanning back from a press that already happened asks one question
of a record that already exists, and a move either has its prefix behind it or it does not. That is also what buys
the latency claim: a completion is stamped with the frame index of the row carrying its terminal press, not the
one after.

```
for each row not yet scanned (oldest first):
    advance every live episode against this row                       # see Deferral
    for each VISIBLE press edge the episodes did not already speak for:
        if the set's deferral verdict for that button is non-zero -> open an episode; continue
        for each candidate in the resolution row (already ordered by priority):
            if scan(candidate, terminal = this row) -> complete it, stop for this button
    settle every episode still waiting on an intent that just completed

scan(intent, terminal row T):
    terminal step must match T                       # buttons: visible press OR held; direction: == T.Octant
    if the intent has one step -> matched
    oldest = w > 0 ? min(T + w - 1, retained - 1) : retained - 1     # offsets; w counts frames INCLUDING T's
    step   = second-to-last
    for offset = T+1 .. oldest:
        if row[offset] matches step:                 # buttons here need a visible PRESS EDGE, not held-ness
            remember its octant; step = previous step
            if no steps left -> matched
        else if Lenient:                             continue
        else if neither neighbouring step names a direction:  continue
        else if row[offset].Octant is the last-matched or the sought step's octant: continue
        else -> no match
    no match
```

- **A step consumes the most recent row that satisfies it and the walk never revisits**, so steps cannot reorder:
  `236` matched against a record holding 6 then 3 then 2 finds them in that order or fails.
- **`w=` counts logic frames INCLUDING the terminal's**, so `w=1` is a move that completes on one row. An
  undeclared window (`0`) is bounded only by what the ring still holds — a step whose row was evicted cannot be
  proven either way, and pretending otherwise would make the answer depend on `_RingCapacity`.
- **`Strict` vs `Lenient` is about intervening OCTANTS and nothing else.** Strict tolerates a row between two
  matched steps only if it holds one of them — which is what makes a direction the player is still HOLDING (the
  common case at 60 Hz, where a deliberate step spans several rows) not break contiguity. Two neighbouring steps
  that name no direction at all constrain nothing: lenience is a statement about unmatched directions, and between
  two buttons there are none to be unmatched.
- **Terminal buttons accept held-ness; prefix buttons do not.** A chord terminal asks whether several buttons were
  down TOGETHER, and a partner pressed a frame earlier is still down on the terminal's row. A prefix step asks for
  an INPUT the player made, and a button that merely stayed down is not another press of it.
- **Chord simultaneity is one logic ROW.** The baked `_ChordWindowFrames` governs how long a press WAITS for its
  partner, not how far apart two atoms may be once both are down: the terminal's pressed-or-held rule is what
  makes a partner pressed a few frames earlier still count, and it is evaluated on a single row.

### The arbiter

The resolution row is already ordered most-dominant first, so **first full match wins the frame for that button**
and there is no second sort here that could disagree with the one the bake settled. Two DIFFERENT buttons
completing different moves on one row both complete: they were never rivals, only neighbours.

### Phases — `Idle / Pending / Completed / Failed`, latched

| Phase | Means | Frame it names |
|---|---|---|
| `Idle` | nothing is happening to this move, or it was in a wait and LOST | — |
| `Pending` | a press that could complete it is being waited out (see *Deferral*) | the press frame |
| `Completed` | it last completed on this frame | the completion frame |
| `Failed` | a wait it was part of resolved and NOTHING matched | the resolution frame |

Transitions out of each: `Idle → Pending` (a wait opens) or `→ Completed` (an unambiguous press);
`Pending → Completed` / `Failed` / `Idle` (won / answered nothing / lost); `Completed` and `Failed` `→ Idle`
(decay), or straight to `Pending`/`Completed` when a new press supersedes them.

`Completed` and `Failed` are both latched, and both DECAY back to `Idle` `_LatchDecayFrames` after the frame they
were stamped on. Until then a poller reads "how did this last resolve, and on which frame" — comparing that frame
index against the frame it last acted on is what turns the latch into an edge. The index is the sampler's own
monotonic logic-frame counter, the same number `FCk_Intent_FrameRecord::Get_FrameIndex` reports.

**Decay is not tidying — it is what bounds a claim.** An undecaying completion stays claimable indefinitely, so a
consumer could take exclusive ownership of a special that landed two fights ago and every other consumer polling
that row would be excluded from a move nobody made. The latch and the claim expire together, and the claim is
cleared by the same transition that clears the phase. `Pending` NEVER decays: an episode carries its own windows,
and a second clock that knew nothing about them would cut a charge short.

**A loser goes back to `Idle`, not to `Failed`.** `Failed` is reserved for a wait that answered nothing at all —
otherwise every tap press would permanently brand its hold sibling, which is noise rather than information. **A
new press supersedes the previous answer** for every candidate on its terminal: opening a wait puts them all in
`Pending`, which clears whatever they were latched at. That is the same rule as a second completion replacing the
first, applied one step earlier.

`TryGet_CompletionFrame*` answers `INDEX_NONE` for every row that is not `Completed` — a `Pending` one, a `Failed`
one, and an intent the set does not carry. Rows hold ONE frame for whichever phase they are in, and a caller that
could read a failure's frame as a completion's would act on a move that did not happen.

`Get_IntentPhase` is the ruled primary read and it is keyed by the definition's `_IntentTag`. The `Intent.*` tag
namespace is still an open question and nothing mints one, so every set baked today carries an EMPTY tag that
matches no row — `Get_IntentPhase_ByName` is what a v1 consumer and every test actually calls. Both answer `Idle`
for an intent the active set does not carry: `Idle` is what a consumer acts on either way, and a phase enum with
an `Unknown` value would make every caller branch on a case only a typo produces.

### Deferral — executing the verdicts

A press whose baked verdict carries a non-zero cause opens an EPISODE: every candidate on that terminal goes
`Pending`, and the press is answered later. Nothing else defers — **the bake stores a verdict only where it found
a forward ambiguity, and sharing a terminal is not one**, so `236+P` beside a bare `P` still completes on the
press frame while a tap/hold pair on one button waits.

An episode remembers the press FRAME, not a ring offset — offsets slide as the ring advances and an episode
outlives several rows by construction. It also snapshots the terminal's resolution row; the set cannot change
under a live episode, because **a swap drops every episode without reporting anything**: a swap is not an answer
to the press that was waiting, it is the question being withdrawn.

**Chord cause** (`_ChordMemberFrames` > 0) — the press might be half of a two-button chord.

- Every row inside the window, the chord candidates (terminal step with more than one BUTTON atom) are scanned
  with the terminal anchored on **that row**, because that is the row the chord would have completed on. First
  match by resolution order wins.
- The window expiring disarms the cause; it does not by itself resolve the episode.

**Hold cause** (`_HoldSiblingFrames` > 0) — the press might be a hold rather than a tap.

- Each row the button is still down and still delivered, the accumulator advances. Any candidate whose
  `hold=` threshold the accumulator has reached is scanned, in resolution order; the first that matches completes
  **on the threshold frame**. Siblings with different thresholds therefore fire at their own thresholds, and the
  first one to win ends the episode.
- The button coming up disarms the cause. So does outlasting the longest threshold any sibling declares with
  nothing matching — there is no hold left that could still win, so the wait is spent.

**When every armed cause is spent**, the episode resolves against the candidates that need no threshold
(`hold=` of zero), with the terminal anchored on the **original press row**. If none matches, every row the
episode put into `Pending` goes `Failed`.

- **The completion frame and the scan anchor are DIFFERENT frames, on purpose.** The completion frame names the
  frame the wait ended on — the latency was really paid and a consumer must not be told otherwise. The backward
  scan behind it is still anchored on the press row — the player's history is what it was and must not be
  rewritten to whatever the stick was doing when the timer expired.
- **Both causes run CONCURRENTLY when a button has both**, and neither is assumed shorter. Whichever answers first
  — a partner arriving or a threshold being reached — resolves the episode. Within one row the chord is asked
  first, because a chord that actually completed on that row is the most specific evidence available and is the
  only branch whose terminal is that row rather than the press row.
- **A completion anywhere settles every episode waiting on it.** Two episodes cannot still be ambiguous between
  candidates one of which has already been decided, so an intent completing on a row purges every other live
  episode that listed it, returning their `Pending` rows to `Idle`.

### The hold accumulator — physical fact vs applied policy

Hold duration is counted in a struct of its own, beside the phase rows, and that separation is the whole of the
rule. The **fact** already exists and is not duplicated: the record's `_Held` says the button is down, frame by
frame, for anyone who asks. What the record cannot say is how long it has been down *as far as this layer is
concerned*, because delivery is a per-layer question — a modal opening over a charge stops the layer from being a
consumer while the player's thumb never moves.

v1 ships **the default policy pair only** ([P5-D6] — the conservative, loud one). The policy enums may exist on
the surfaces; nothing else is constructible until a consumer demands it, and the notation grows no modifier.

| Transition | Policy | What happens |
|---|---|---|
| delivery LOST mid-episode | `Cancel` | the episode ends, every row it was waiting on goes `Failed`, the accumulator is dropped |
| delivery REGAINED | `RequireRePress` | nothing re-arms. A button that is merely still physically down starts no new wait — only a fresh visible press edge opens an episode |

Deliverability is evaluated LIVE, by asking the question routing would ask: does any layer above this one
currently declare a `Consume` capture that would end the walk for that key. A held button routes nothing after
its press edge, so there is no row to read the answer off — and the mask can appear on any frame of a hold, which
is precisely the case the policy exists for.

### The claim — an IMMEDIATE mutator, and why

Polling is a read, so nothing about it stops two consumers acting on the same completion. `Request_Claim` is what
makes ownership exclusive, and it is **the house immediate-mutator escape hatch, declared here with its reason:
a deferred claim makes same-frame double-claim a race, and two pollers must observe the first claim
synchronously.** The mutation lands on the calling stack and the completion delegate fires after it.

| Situation | Result |
|---|---|
| `Completed`, unclaimed | `Succeeded`, claim stamped with the claimant and the current record frame |
| `Completed`, already held by the SAME claimant | `Succeeded`, stamp untouched — idempotent, the caller's intent already holds |
| `Completed`, held by ANOTHER entity | `Failed` — the exclusion working, not an error |
| `Idle` / `Pending` / `Failed` | `Failed` — there is nothing completed to claim |
| a name the active set does not carry | `Failed` |
| invalid matcher or invalid claimant | `Failed_NotEnqueued` — rejected at the boundary, nothing touched |

- **A new completion clears the claim.** A fresh press is a fresh event; a stale claim riding across it would let
  one consumer's old ownership block everybody from the next completion.
- **Per-layer by construction.** Each matcher's rows are its own, so `PassThrough` layers stay honest for free and
  claiming through a mask is unrepresentable — a layer that never received the press never completed the intent.
- **There is no down-stack claim.** That request is `handled == true` reborn; the mechanism for "block the layers
  below me" remains a capture edit.
- **Two consumers on the SAME layer are ordered by processor order.** Accepted and documented: an intent that
  needs single-consumer semantics lives behind one consumer.

### Two surfaces — the poll is the authority, the signals are presentation

**Late binders under `FireIfPayloadInFlight*` receive only the LAST payload — sequences are reconstructed from the
poll surface and the frame record, never from signal replay.** That is the law, and it is restated in the
`BindTo_*` doc comments so nobody re-learns it: nothing here buffers more than one payload, so a consumer that
tried to rebuild a history from replay would be reading one the matcher does not keep. Poll for state, listen for
presentation.

Two signals, both on the MATCHER entity, both via `CK_DEFINE_SIGNAL_AND_UTILS_WITH_DELEGATE`:

| Signal | Payload | Fires |
|---|---|---|
| `OnIntentPhaseChanged` | matcher, intent name, intent tag, previous phase, new phase, frame | every transition, decay's `→ Idle` included |
| `OnIntentCompleted` | matcher, intent name, intent tag, frame | every transition INTO `Completed`, re-completions included |

- **Identity rides the PAYLOAD; there is no entity per intent and there will not be one.** A consumer binds on the
  matcher handle it already holds and filters by name in the handler. Minting an entity per definition would buy a
  narrower subscription at the cost of a lifetime for every move a character can express, and of every consumer
  having to resolve one before it could listen at all.
- **Both phases are carried** because a transition is the interesting thing, not the destination:
  `Completed → Idle` is a latch expiring and `Pending → Idle` is a move losing arbitration, and a payload naming
  only the new phase could not tell them apart. `OnIntentCompleted` is a strict subset of that and exists anyway,
  so "play the animation when the special lands" needs no two-phase comparison a consumer can get wrong.
- **Default binding policy is `FireIfPayloadInFlightThisFrame`** — the house default, and the only policy that
  cannot make the signal contradict the poll, since a latch cannot decay inside the frame it was stamped on.
  `FireIfPayloadInFlight` is the right choice for a consumer that spawns a frame or two after the input that
  concerns it; the caveat is that the replayed payload may name a completion the poll surface has already decayed
  away, so check the frame it carries.

### One writer, structurally

`FIntentMatcher_PhaseWriter::Set_Phase` is the only thing in the module that can move a phase, and it broadcasts
from inside itself. `FIntentMatcher_PhaseRow` names that writer as the only friend reaching its phase fields — the
processors cannot write them at all — so **a phase cannot change without its signal** is enforced by the compiler
rather than by everyone remembering. Every path comes through it: a wait opening, a move completing, a loser
settling, an episode failing, a latch decaying, and the outgoing rows of a set being swapped out.

A call that changes neither the phase nor the frame is a silent no-op. A RE-completion on a later frame is not:
same phase, new frame, real event, and both signals fire. That distinction is why the writer compares the frame as
well as the phase — firing on every unchanged tick would have a consumer retrigger an animation every frame, and
firing on none would swallow the second use of a move.

**A swap signals its outgoing rows to `Idle` before discarding them.** An intent the active set no longer carries
polls as `Idle`, so the observable phase moved and the signal owes an account of it — the two surfaces are two
views of one row, never two sources of truth. (The pending EPISODES a swap drops are silent, which is a different
question: a swap is not an answer to the press that was waiting, it is the question being withdrawn.)

**Every payload's frame is a real record frame, with ONE exception: a swap's `→ Idle` carries `INDEX_NONE`.** A
swap drains a group before the sampler writes this frame's row, so there is no frame the transition happened on
without inventing one. Every other transition — including decay's, which is measured in rows — names the row it
landed on, and a consumer that measures a gap from a payload frame can rely on that everywhere else.

### Scan diagnostics — the near-miss ring

A failed backward scan leaves **no trace anywhere else**. The matcher declines to complete and that is all: the
record shows the press, the phase rows show `Idle`, no signal fires, and "I did the motion and nothing came out"
is unanswerable. The ring is the only surface that can say WHICH step had nothing behind it and by how many
frames it missed.

**Opt-in, off by default**, via the CVar `ck.Intent.RecordScanDiagnostics`. Read once per pass into the scan
context, never per scan — the switch cannot change mid-tick and a per-scan console lookup is the cost the opt-in
exists to avoid. `Get_ScanDiagnosticsEnabled()` reports it; the switch is global rather than per-matcher because
it is a debugging control, and a per-matcher one would leave a reader wondering which matcher they forgot to arm.

**Cost model.** One entry per COMPLETED scan attempt, both outcomes. That is cheap for the ordinary press (one
attempt) and deliberately not free during a deferral: a chord window polls its candidates once per frame, so a
wide window fills the ring with near-identical failures. That is the honest record of what the matcher did, and
it is why the ring is 32 entries and why writing it is switched off by default. When off, the only work is a
stack-constructed entry whose `TArray` never allocates.

| `FCk_Intent_ScanDiagnostic` | |
|---|---|
| `_IntentName` / `_IntentTag` | which move was being scanned |
| `_TerminalFrame` | the record frame the scan anchored on |
| `_Outcome` | `Matched` / `FailedAtStep` |
| `_FailedStepIndex` | the definition index the walk died on, `INDEX_NONE` on a match |
| `_Steps` | the steps walked, in WALK order |

| `FCk_Intent_ScanStepDiagnostic` | |
|---|---|
| `_StepIndex` | index into the DEFINITION's steps — the walk runs backwards, so this is what puts them back in authored order |
| `_Outcome` | `Matched` / `NotSatisfied` / `WindowExhausted` / `ContiguityBroken` |
| `_MatchedAtFrame` | the row it matched on, `INDEX_NONE` otherwise |
| `_FramesExamined` | rows read while seeking this step, including the one that matched |

- **`_Steps` is in WALK order — terminal first — and stops where the scan died.** A scan that failed three steps
  from the front has nothing to say about the two it never looked for, and inventing entries for them would be
  reporting work that did not happen.
- **The three failures are three different fixes.** `NotSatisfied` (terminal only) means the press itself was
  wrong for this move — the direction was not held, the chord's partner was not down. `WindowExhausted` means the
  input was there in spirit and the player was too slow, or the ring stopped retaining that far back: a tuning
  answer, read against the definition's `w=`. `ContiguityBroken` means a Strict move saw the stick pass through a
  direction it does not mention: a lenience answer.
- **`Get_ScanDiagnostics` answers NEWEST FIRST**, the order a near-miss list is read in.
- **Recorded at every scan site**, which is two call sites covering four paths: the immediate arbiter scan, and
  the episode resolver that serves the chord branch, the hold branch and the final post-wait resolution. Both go
  through one wrapper so the ring cannot end up describing only some of them.
- **Deferral EPISODE outcomes are NOT entries.** A window that expired with no partner already appears as the
  chord candidate's failed scans, one per polled frame; a delivery-loss cancel ran no scan at all, so it has no
  steps and nothing the per-step shape could carry. A step-less entry in a per-step ring would make every reader
  branch on "is this a scan or a notice". Cancellation is already answered by the `Failed` phase and its
  `OnIntentPhaseChanged` payload, which is the surface that owns it.
- **The ring is BESIDE the phase rows, not in them.** A row is one definition's current standing; a diagnostic is
  one finished attempt. Different lifetimes, and a row carrying its own history would grow without bound.
- **A set swap does not clear the ring.** The entries describe scans that really happened, and a debugger reading
  a near miss from just before a swap is reading a true thing.

### Captures follow the SET, and rebinds follow the MAP

Activating a set registers a `Key` capture on the matcher's own layer for each of the set's terminal buttons,
resolved through the source's `InputButtonMap`. Behaviour is the matcher's `_CaptureBehavior` (default `Consume`).

- **The swap is ATOMIC.** Every terminal is resolved first; a single one that resolves to no key — a button the
  map never minted, or a mapped button the player left unbound — rejects the WHOLE swap with `Failed`, leaving the
  previous set active and its captures untouched. That is a RESULT rather than a diagnostic on purpose: an unbound
  mapping is a state a player can produce from a settings screen, so the caller is told and decides. A definition
  naming a button that does not exist at all was already rejected at bake time and cannot reach here.
- **A default-constructed set DEACTIVATES** — captures removed, rows cleared — and completes `Succeeded`, because
  that is what the caller asked for.
- **Capture edits are the ordinary deferred kind.** The matcher enqueues them through the layer's own
  `Request_AddCapture` / `Request_RemoveCapture`, which drain in `FGroup_Input_Collect` — one group BEFORE routing
  and one frame after this. So `Get_Captures` on the layer does not show a just-registered row, and the matcher
  never consults it: it keeps its own `_RegisteredCaptures` and answers `Get_RegisteredCaptureKeys` from that.
  Nothing about matching depends on the capture being live, since an uncaptured press is recorded `PassedThrough`
  and `PassedThrough` is visible.
- **Rebinds are detected by POLLING, per tick.** The button map re-derives without telling anyone, so each pass
  compares the map's current answer for every registered terminal against what the matcher last registered and
  edits only the difference. A handful of comparisons per matcher, and deliberately not a delegate: a processor
  binding a UObject delegate would own a lifetime it has no way to end. The edits land on the next routing pass,
  the same one-frame contract every capture edit obeys.
- **Edits are DIFFED, never remove-all-then-add-all**, because two terminal buttons may legitimately resolve to
  one key and a blanket removal would drop a capture the surviving button still needs.
- **A terminal whose button becomes unbound goes inert**: its capture is removed, it matches nothing, and it comes
  back on its own the moment the map resolves it again. One `Verbose` line per transition — the log fires only
  where the comparison actually changed, so a terminal that stays unbound is silent after the first line.

---

## Processor groups — after routing, before gameplay

Three groups (`CkIntent_ProcessorGroups.h`) — Collect and Sample registered in `CkIntentSampler_Processor.cpp`,
Match in `CkIntentMatcher_Processor.cpp`:

```
FGroup_Input_Collect → FGroup_Input_Bias → FGroup_Input_Route → FGroup_Intent_Collect → FGroup_Intent_Sample → FGroup_Intent_Match → FGroup_Gameplay
```

- **`FGroup_Intent_Collect` `RunAfter FGroup_Input_Route`** — the router DRAINS the source inbox and retains the
  per-event delivery outcomes only for the remainder of that render frame. Running before it would find nothing.
- **`FGroup_Intent_Collect` `RunBefore FGroup_Intent_Sample`** — buffer, then claim. **This is a GROUP edge, not
  two processors sharing a group, and that is deliberate:** registration order within one group is not an
  ordering guarantee here, so a setup-before-consumer relationship has to be expressed as two groups or it is
  not expressed at all.
- **`FGroup_Intent_Sample` `RunBefore FGroup_Gameplay`** — a consumer reading the record acts on the frame the
  input arrived on rather than the one after.
- **`FGroup_Intent_Match` `RunAfter FGroup_Intent_Sample`, `RunBefore FGroup_Gameplay`** — the scan's whole input
  is the row Sample appends, so it cannot be a second processor sharing Sample's group for the same reason
  Collect is not. A consumer polling a phase from gameplay reads this frame's answer.
- **Set swaps drain in `FGroup_Intent_Collect`,** two groups ahead of the scan. That is what makes the ordering a
  guarantee rather than a hope: a set activated this frame is the set this frame's scan runs, and a matcher can
  never match half of one set and half of another.

### Matching cadence — unrated, over a rated record

The sampler writes one row per fixed 60 Hz logic frame; `FProcessor_IntentMatcher_Match` is UNRATED and runs every
render frame. Rating it to the same cadence would put two independently-phased accumulators in one pipeline with
no defined alignment between them. Instead the matcher remembers the last record frame index it consumed and
scans everything newer, **oldest first** (a later row's press may complete a move whose prefix a nearer row
carries). Above 60 fps most passes find no new row and do nothing; below it several rows arrive at once and every
one of them is scanned — no row is matched twice and none is skipped, at any frame rate.

A matcher that has never scanned takes only the NEWEST row, so a set activated now cannot complete on presses the
player made before it existed. The backlog is clamped to what the ring still holds.

**Pending episodes and latch decay ride the same pass, and there is no second processor and no group of their
own.** An episode is a fact about rows — a hold is counted in them, a chord window is measured in them, a latch
expires after a number of them — and this is the one place rows are read. Giving either its own processor would
mean two readers of the same ring with no ordering between them, and a hold or a decay whose count depended on
which of them ran first.

Decay is evaluated LAST on every row, after the presses that row carries have been resolved. That ordering is what
stops a completion stamped this frame being decayed by the same pass that produced it, and it is what makes the
decay frame exact under catch-up: several rows arriving at once each get their own decay check rather than one
check against whichever frame the renderer caught up to.

---

## Anti-patterns

1. **Don't read the record from a group that isn't ordered after `FGroup_Intent_Sample`.** The ring itself is
   durable, but "the latest row" means a different frame depending on where you read it. Gameplay is safe by
   group edge; anything inside the input groups is reading last frame's latest.
2. **Don't treat `_Held` as a list of keys.** It is button space, resolved fresh every row, so a rebind that
   lands mid-hold moves what the row names. If you need the physical key, read `_RoutedEvents`.
3. **Don't assume a row exists.** `Add` composes the feature; the first row lands on the next sampling pass, and
   an injected event lands one logic tick after the render frame that routed it. `Get_FrameCount` is the check
   for the first, a named wait on the row's contents is the check for the second — never a bare frame hop.
4. **Don't infer "nobody received it" from `PassedThrough`.** That outcome means no layer ENDED the walk — every
   `PassThrough` capture on the way down was still delivered. See `CkInput/CLAUDE.md`.
5. **Don't raise `_RingCapacity` to keep history "just in case".** The ring is a working window for matching, not
   a session log; a consumer that needs the whole session should record rows out of it as they arrive.
6. **Don't compose two samplers on one source.** `Add` rejects it — two rings would produce two disagreeing
   frame numberings for the same input, and nothing could say which one a stored offset referred to.
7. **Don't re-derive the octant from a row's axis pair.** The row's `_Octant` is hysteresis-damped against the
   PREVIOUS row; a consumer that recomputes `round(atan2 / 45)` from `_ConditionedAxisX`/`Y` gets the undamped
   candidate and will disagree with the record on exactly the frames the damping exists for. Same for the cleaned
   pair against `_Held`: that answer needs a press ORDER the row does not carry.
8. **Don't read `_CleanedHorizontal` as "which way the player is holding".** It is the cleaned CARDINAL state,
   derived only from the quad's buttons — a player on a stick moves `_Octant` and leaves both cleaned slots at
   `Neutral`. A consumer that wants "a direction, from whatever the player is using" reads both and says which
   wins; the record deliberately does not choose for it.
9. **Don't hand-assemble an `FCk_Intent_Definition`.** You cannot — and the missing constructor is the feature,
   not an oversight. A definition built beside the parser would be a second dialect nothing could tell apart from
   an authored one, and the notation would stop being the single description of what a move is.
10. **Don't read a definition without reading the outcome first.** A rejected `FCk_Intent_ParseResult` carries an
    empty definition, so a caller that skips `Get_Outcome` sees a move with no steps rather than an error, and the
    typo ships.
11. **Don't resolve a button name at parse time.** The notation says `LP`; only the bake is given the
    name → ButtonId table. Resolving early either invents an identity — a second minting authority beside the
    ButtonMap's — or rejects a perfectly well-formed notation for naming a button declared elsewhere.
12. **Don't rebake to add or remove one move.** Every table in a set is derived from every intent in it, so there
    is no incremental edit that leaves the rest coherent. Bake the sets a character can be in up front and SWAP
    them — that is what makes activation an assignment instead of a rebuild.
13. **Don't treat an absent deferral row as missing data.** No row IS the answer: this button waits for nothing.
    A caller that special-cases "not found" is re-implementing the default the set deliberately does not store,
    and will diverge from `Get_DeferralVerdict` the first time the rules change.
14. **Don't defer a press because its button appears mid-sequence somewhere.** Sequence-suffix membership never
    defers, and the bake never even visits a non-terminal step to decide it. A matcher that adds its own wait
    there re-introduces exactly the input latency this model was shaped to avoid.
15. **Don't read `Get_IntentPhase` as an edge.** `Completed` is LATCHED, so a poller that branches on the phase
    alone acts on the same completion every frame until it decays. Compare `TryGet_CompletionFrame*` against the
    frame you last acted on — or bind `OnIntentCompleted`, which IS the edge.
16. **Don't compose a matcher on anything but the layer whose arbitration you mean.** `Add` rejects a non-layer,
    but it cannot reject a layer at the wrong PRIORITY — and priority is what decides which events the set is
    even allowed to see. A move set on a layer above your modals will keep matching while the modal is up.
17. **Don't register your own captures for a matcher's terminals.** Captures follow the SET: the swap registers
    them, a rebind moves them, and a deactivating swap removes them. A hand-added capture for the same key
    survives the removal and keeps masking the layers below after the set is gone.
18. **Don't read the layer's `Get_Captures` to find out what a matcher is capturing.** A capture edit is deferred,
    so the layer does not carry a just-registered row for a frame. `Get_RegisteredCaptureKeys` on the matcher is
    the answer as of the swap or re-resolution that decided it.
19. **Don't infer "the set does not contain this move" from an `Idle` phase.** An unknown name and a known-but-
    uncompleted move answer identically, deliberately. Ask `Get_ActiveIntentCount` / `Get_HasActiveSet` if what
    you need to know is whether a set is loaded.
20. **Don't read a `Pending` row as "about to complete".** It means the press is genuinely ambiguous and the
    matcher has not chosen — the answer may be a different move, or `Failed`. A consumer that acts on `Pending`
    has re-introduced the guess deferral exists to avoid.
21. **Don't rely on a swap landing on the calling stack.** `Request_SwapSet` is deferred and drains in
    `FGroup_Intent_Collect`; polling right after it returns reads the OLD set. Wait on `Get_HasActiveSet` /
    `Get_ActiveIntentCount`, or bind the completion delegate — which is also the only way to learn that the swap
    was REJECTED for an unresolvable terminal.
22. **Don't add `hold=` to one move on a terminal and expect its siblings to keep their timing.** A hold sibling
    is one of the two things that makes a press defer, so declaring it costs EVERY move on that button the wait —
    the bare tap now answers on release rather than on the press. That is the correct behaviour and it is also a
    latency change to a move nobody edited; check the terminal's other definitions before adding one.
23. **Don't measure a hold with your own timer against `_Held`.** The record's held set is the physical fact and
    keeps counting through a mask; the matcher's accumulator is the policy-applied count and stops. A consumer
    that rolls its own will complete charges out of menus, which is exactly what the default `Cancel` policy is
    there to prevent.
24. **Don't treat a `Failed` row as "this move is broken".** It is the honest answer to an ambiguous press that
    matched nothing — a mistimed input, or a charge a modal interrupted. It latches like `Completed` and is
    cleared by the next press on that terminal.
25. **Don't claim an intent you have not confirmed `Completed` this frame.** A claim on a latched OLD completion
    succeeds exactly as one on a fresh completion does, because a latch carries no freshness of its own. Read
    `TryGet_CompletionFrame*` against the frame you last acted on FIRST, then claim.
26. **Don't defer the claim behind your own request queue.** Its whole value is that it is immediate — two
    pollers must see the first claim synchronously. Wrapping it in something deferred re-opens the race it
    closes.
27. **Don't rebuild a sequence of transitions from signal replay.** A late binder receives the LAST payload and
    nothing before it. The poll surface and the frame record are the authority; the signals are presentation.
28. **Don't act on a replayed `OnIntentCompleted` without checking its frame.** Under `FireIfPayloadInFlight` the
    payload may name a completion that has since decayed — the poll surface will say `Idle` while the handler is
    holding a completion frame. Compare it against the current record frame before doing anything with it.
29. **Don't raise `_LatchDecayFrames` to "make polling easier".** The window is what bounds a CLAIM: the longer a
    completion stands, the longer a consumer can take exclusive ownership of a move the player made ages ago. If a
    consumer needs longer, it should be remembering the completion frame itself, not asking the matcher to.
30. **Don't write a phase row outside `FIntentMatcher_PhaseWriter`.** The friend list makes it impossible today;
    if a future change adds a friend to get around it, the module has silently lost "a phase cannot change
    without its signal" — and the two surfaces start disagreeing in ways only a consumer notices.
31. **Don't bind `OnIntentCompleted` from AngelScript with a signature that merely looks right.** A dynamic
    delegate is matched by SIGNATURE: one parameter declared differently and the bind silently never fires — no
    compile error, no warning. Copy the parameter list from `FCk_Delegate_IntentMatcher_Completed` verbatim.
32. **Don't ship gameplay that reads `Get_ScanDiagnostics`.** It is empty unless someone turned the CVar on, so
    anything built on it works on the developer's machine and nowhere else. It exists for a debugger and a human.
33. **Don't leave `ck.Intent.RecordScanDiagnostics` on.** Every scan attempt writes an entry, and a chord window
    polls once per frame — a session left recording is paying for a ring nobody is reading.
34. **Don't read `_FramesExamined` as "how late the player was".** It is how many rows the WALK read, which is
    bounded by `w=` and by the ring's retention. A step that missed by a hundred frames inside a ten-frame window
    reports nine, because nine is all the walk was ever allowed to look at.

---

## See also

- `CkInput/CLAUDE.md` — the raw layer this samples: the inbox, layer arbitration, the delivery-outcome
  retention (*Delivery visibility*), axis conditioning, and the button space.
- `CkEcs/Claude.md` — processor groups, the `TickRate` / `TickCatchUpPolicy` / `MaxReplayedTicks` cadence traits.
