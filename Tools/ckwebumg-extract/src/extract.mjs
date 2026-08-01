#!/usr/bin/env node
// ckwebumg-extract — HTML+CSS mockup -> *.ckui.json (schema v1) + golden PNG.
// Editor-time tooling only; nothing here ships in a cooked build.
//
// Design constraints (CampaignBrief.md §5, Gate_01):
//  - IR contains computed, resolved, absolute values only — no CSS syntax.
//  - Deterministic: same input + same browser version => byte-identical JSON.
//  - No silent property drops: author-set properties outside the v1 surface land in
//    `unsupported[]` with stylesheet provenance (file:line) via CSS.getMatchedStylesForNode.

import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';
import puppeteer from 'puppeteer-core';

const VIEWPORT = { width: 1920, height: 1080, deviceScaleFactor: 1 };
const SCHEMA_VERSION = 1;

const CHROME_CANDIDATES = [
    'C:/Program Files/Google/Chrome/Application/chrome.exe',
    'C:/Program Files (x86)/Google/Chrome/Application/chrome.exe',
];

// DECISION 4 proposed v1 surface (CampaignBrief.md §4). Longhands, as Chromium reports them.
// Everything an author sets outside this list is diagnosed, never dropped silently.
const SUPPORTED_PROPERTIES = new Set([
    // box model
    'width', 'height', 'min-width', 'min-height', 'max-width', 'max-height',
    'margin-top', 'margin-right', 'margin-bottom', 'margin-left',
    'padding-top', 'padding-right', 'padding-bottom', 'padding-left',
    'box-sizing',
    // flex layout
    'display', 'flex-direction', 'justify-content', 'align-items', 'align-self',
    'align-content', 'flex-wrap', 'flex-grow', 'flex-shrink', 'flex-basis',
    'row-gap', 'column-gap', 'order',
    // positioning
    'position', 'top', 'right', 'bottom', 'left', 'z-index',
    // paint
    'background-color', 'background-image', 'background-size', 'background-position',
    'background-repeat',
    'border-top-width', 'border-right-width', 'border-bottom-width', 'border-left-width',
    'border-top-style', 'border-right-style', 'border-bottom-style', 'border-left-style',
    'border-top-color', 'border-right-color', 'border-bottom-color', 'border-left-color',
    'border-top-left-radius', 'border-top-right-radius',
    'border-bottom-right-radius', 'border-bottom-left-radius',
    'box-shadow', 'opacity', 'overflow-x', 'overflow-y', 'transform', 'transform-origin',
    'visibility',
    // typography
    'color', 'font-family', 'font-size', 'font-weight', 'font-style',
    'line-height', 'letter-spacing', 'text-align', 'text-overflow', 'white-space',
    'text-decoration-line', 'text-transform', 'vertical-align', 'word-break',
    'white-space-collapse', 'text-wrap-mode', 'text-wrap',
    // misc accepted-as-computed
    'cursor', 'pointer-events', 'object-fit',
]);

// Shorthands expand to longhands in matched rules' parsed properties; accept the shorthand
// spellings too so a rule like `margin: 8px` is not misdiagnosed.
const SUPPORTED_SHORTHANDS = new Set([
    'margin', 'padding', 'border', 'border-width', 'border-style', 'border-color',
    'border-top', 'border-right', 'border-bottom', 'border-left', 'border-radius',
    'background', 'flex', 'flex-flow', 'gap', 'inset', 'overflow', 'font',
    'text-decoration', 'place-items', 'place-content',
]);

const STATE_PSEUDO_CLASSES = ['hover', 'active', 'disabled'];

const KNOWN_CK_ATTRIBUTES = new Set([
    'data-ck-name', 'data-ck-widget', 'data-ck-bind', 'data-ck-slot', 'data-ck-ignore',
]);

// Value-level restrictions on properties whose *name* is supported. A supported property
// carrying an out-of-surface value (display:grid) must be diagnosed like any other drop.
const VALUE_RULES = new Map([
    ['display', new Set(['flex', 'inline-flex', 'block', 'inline-block', 'inline', 'flow-root', 'none'])],
    ['position', new Set(['static', 'relative', 'absolute'])],
]);

// Semantic DOM attributes the emitter needs (form-control state, alt text). Captured
// verbatim; boolean attributes report true.
const SEMANTIC_ATTRIBUTES = ['type', 'value', 'placeholder', 'disabled', 'checked', 'alt', 'href'];

// ---------------------------------------------------------------------------------------------

const findChrome = () => {
    const hit = CHROME_CANDIDATES.find(p => fs.existsSync(p));
    if (!hit) {
        console.error('ERROR: Chrome not found. Checked:\n' + CHROME_CANDIDATES.join('\n'));
        process.exit(2);
    }
    return hit;
};

const px = v => {
    const n = parseFloat(v);
    return Number.isFinite(n) ? round2(n) : 0;
};
// Absolute-px-or-null for constraint values: 'none'/'auto' (and anything non-px) become null.
const pxOrNull = v => (typeof v === 'string' && v.endsWith('px')) ? round2(parseFloat(v)) : null;
const round2 = n => Math.round(n * 100) / 100;

// Quad = [x1,y1,x2,y2,x3,y3,x4,y4] clockwise from top-left. Axis-aligned assumption is
// checked by the caller (transforms make quads non-rectangular; we record both).
const quadToRect = q => ({
    x: round2(Math.min(q[0], q[2], q[4], q[6])),
    y: round2(Math.min(q[1], q[3], q[5], q[7])),
    w: round2(Math.max(q[0], q[2], q[4], q[6]) - Math.min(q[0], q[2], q[4], q[6])),
    h: round2(Math.max(q[1], q[3], q[5], q[7]) - Math.min(q[1], q[3], q[5], q[7])),
});

// Radial prelude ("circle at 40% 40%", "ellipse farthest-side at center", explicit px radii) →
// absolute {center:[cx,cy], radius:[rx,ry]} px against the painted box, per the CSS sizing rules.
// Anything outside the supported grammar returns null — the consumer diagnoses, never guesses.
const parseRadialPrelude = (prelude, boxW, boxH) => {
    const m = /^(?:(circle|ellipse)\s*)?(?:(closest-side|closest-corner|farthest-side|farthest-corner|[\d.]+px(?:\s+[\d.]+px)?)\s*)?(?:at\s+(.+))?$/
        .exec(prelude.trim());
    if (!m) return null;
    const explicitRadii = m[2] && m[2].endsWith('px') ? m[2].split(/\s+/).map(parseFloat) : null;
    // CSS default shape: ellipse, unless a single explicit radius forces circle.
    const shape = m[1] ?? (explicitRadii && explicitRadii.length === 1 ? 'circle' : 'ellipse');
    const size = explicitRadii ? null : (m[2] ?? 'farthest-corner');

    const resolveCoord = (token, basis) => {
        if (token === 'center') return basis / 2;
        if (token === 'left' || token === 'top') return 0;
        if (token === 'right' || token === 'bottom') return basis;
        if (token.endsWith('%')) return parseFloat(token) / 100 * basis;
        if (token.endsWith('px')) return parseFloat(token);
        return null;
    };
    let cx = boxW / 2, cy = boxH / 2;
    if (m[3] !== undefined) {
        const posTokens = m[3].trim().split(/\s+/);
        if (posTokens.length > 2) return null;
        cx = resolveCoord(posTokens[0], boxW);
        cy = resolveCoord(posTokens[1] ?? 'center', boxH);
        if (cx === null || cy === null) return null;
    }

    let rx, ry;
    if (explicitRadii) {
        rx = explicitRadii[0];
        ry = explicitRadii.length === 2 ? explicitRadii[1] : explicitRadii[0];
    } else {
        const sideX = { 'closest-side': Math.min(cx, boxW - cx), 'farthest-side': Math.max(cx, boxW - cx) };
        const sideY = { 'closest-side': Math.min(cy, boxH - cy), 'farthest-side': Math.max(cy, boxH - cy) };
        const dx = size.includes('closest') ? sideX['closest-side'] : sideX['farthest-side'];
        const dy = size.includes('closest') ? sideY['closest-side'] : sideY['farthest-side'];
        if (size.endsWith('corner')) {
            // Corner sizes keep the side-size aspect and scale through the corner (k = sqrt2 when
            // dx,dy are the corner deltas): circle uses the corner distance directly.
            if (shape === 'circle') { rx = ry = Math.hypot(dx, dy); }
            else { rx = dx * Math.SQRT2; ry = dy * Math.SQRT2; }
        } else {
            if (shape === 'circle') { rx = ry = size === 'closest-side' ? Math.min(dx, dy) : Math.max(dx, dy); }
            else { rx = dx; ry = dy; }
        }
    }
    if (!Number.isFinite(rx) || !Number.isFinite(ry)) return null;
    return { center: [round2(cx), round2(cy)], radius: [round2(rx), round2(ry)] };
};

// Typed gradient parse of a COMPUTED background-image (colors already rgb()/rgba()).
// Returns {gradientType, angleDeg, stops:[{rgba, posPct|null}]} or {gradientType:'unparsed'} —
// the verbatim `computed` field is always kept alongside, so nothing is ever lost.
const parseGradient = (v, boxW, boxH) => {
    const m = /^(repeating-)?(linear|radial|conic)-gradient\((.*)\)$/s.exec(v);
    if (!m || m[1]) return { gradientType: 'unparsed' };
    const kind = m[2];
    // split top-level commas
    const parts = [];
    let depth = 0, cur = '';
    for (const ch of m[3]) {
        if (ch === '(') depth++;
        if (ch === ')') depth--;
        if (ch === ',' && depth === 0) { parts.push(cur.trim()); cur = ''; }
        else cur += ch;
    }
    parts.push(cur.trim());

    let angleDeg = 180; // CSS default: to bottom
    let stopParts = parts;
    let radial = null;
    if (kind === 'linear' && /^-?[\d.]+deg$/.test(parts[0])) {
        angleDeg = parseFloat(parts[0]);
        stopParts = parts.slice(1);
    } else if (kind !== 'linear' && !parts[0].startsWith('rgb')) {
        if (kind === 'radial') radial = parseRadialPrelude(parts[0], boxW, boxH);
        stopParts = parts.slice(1); // conic prelude (and unparseable radial) stays `computed`-only
    } else if (kind === 'radial') {
        radial = parseRadialPrelude('', boxW, boxH); // no prelude: ellipse farthest-corner at center
    }

    const stops = [];
    for (const part of stopParts) {
        const sm = /^(rgba?\([^)]*\))(?:\s+([\d.]+)%)?$/.exec(part);
        if (!sm) return { gradientType: 'unparsed' };
        const rgba = parseColor(sm[1]);
        if (!rgba) return { gradientType: 'unparsed' };
        stops.push({ rgba, posPct: sm[2] !== undefined ? round2(parseFloat(sm[2])) : null });
    }
    if (stops.length < 2) return { gradientType: 'unparsed' };
    return {
        gradientType: kind,
        angleDeg: kind === 'linear' ? angleDeg : null,
        ...(radial !== null ? radial : {}),
        stops,
    };
};

// Typed box-shadow parse of the COMPUTED value: comma-separated layers, each
// "<color> <offsetX>px <offsetY>px <blur>px <spread>px [inset]" (Chromium's emission order).
// Returns [{rgba, offset:[x,y], blur, spread, inset}] or null — the verbatim string is kept
// alongside either way.
const parseBoxShadow = v => {
    const layers = [];
    let depth = 0, cur = '', parts = [];
    for (const ch of v) {
        if (ch === '(') depth++;
        if (ch === ')') depth--;
        if (ch === ',' && depth === 0) { parts.push(cur.trim()); cur = ''; }
        else cur += ch;
    }
    parts.push(cur.trim());
    for (const part of parts) {
        const m = /^(rgba?\([^)]*\))\s+(-?[\d.]+)px\s+(-?[\d.]+)px\s+(-?[\d.]+)px\s+(-?[\d.]+)px(\s+inset)?$/
            .exec(part);
        if (!m) return null;
        const rgba = parseColor(m[1]);
        if (!rgba) return null;
        layers.push({
            rgba,
            offset: [round2(parseFloat(m[2])), round2(parseFloat(m[3]))],
            blur: round2(parseFloat(m[4])),
            spread: round2(parseFloat(m[5])),
            inset: m[6] !== undefined,
        });
    }
    return layers.length > 0 ? layers : null;
};

const parseColor = v => {
    // Chromium computed colors arrive as rgb(r, g, b) / rgba(r, g, b, a). Absolute already.
    const m = /^rgba?\(([\d.]+),\s*([\d.]+),\s*([\d.]+)(?:,\s*([\d.]+))?\)$/.exec(v);
    if (!m) return null;
    return [Number(m[1]), Number(m[2]), Number(m[3]), m[4] === undefined ? 255 : Math.round(Number(m[4]) * 255)];
};

// ---------------------------------------------------------------------------------------------

class Extractor {
    constructor(cdp, pageUrl, imageMeta) {
        this.cdp = cdp;
        this.pageUrl = pageUrl;
        this.baseUrl = pageUrl.slice(0, pageUrl.lastIndexOf('/') + 1);
        this.imageMeta = imageMeta; // src(resolved) -> {naturalWidth, naturalHeight}
        this.nodeCounter = 0;
        this.stylesheetHeaders = new Map(); // styleSheetId -> {sourceURL}
        this.assets = new Map();      // resolved src -> {id, src, kind, intrinsic}
        this.diagnostics = [];        // page-level issues (duplicate names, unknown ck attrs)
        this.ckNames = new Map();     // data-ck-name -> first node id
        this.domNodeIds = new Map();  // IR id -> CDP nodeId, for the untransformed box pass
    }

    assetRef(rawUrl) {
        let resolved;
        try { resolved = new URL(rawUrl, this.pageUrl).href; } catch { resolved = rawUrl; }
        if (!this.assets.has(resolved)) {
            const meta = this.imageMeta.get(resolved);
            this.assets.set(resolved, {
                id: `img${this.assets.size}`,
                src: rawUrl, // rewritten to the normalized copy before the IR is written
                origSrc: rawUrl,
                resolvedUrl: resolved,
                kind: rawUrl.startsWith('data:') ? 'data-uri' : 'raster',
                intrinsic: meta ? [meta.naturalWidth, meta.naturalHeight] : null,
            });
        }
        return this.assets.get(resolved).id;
    }

    async run() {
        const { root } = await this.cdp.send('DOM.getDocument', { depth: -1, pierce: false });
        const html = root.children.find(c => c.nodeName === 'HTML');
        const body = html.children.find(c => c.nodeName === 'BODY');
        return await this.extractElement(body);
    }

    computedMap(list) {
        const m = new Map();
        for (const { name, value } of list) m.set(name, value);
        return m;
    }

    attrMap(node) {
        const m = new Map();
        const a = node.attributes ?? [];
        for (let i = 0; i < a.length; i += 2) m.set(a[i], a[i + 1]);
        return m;
    }

    ruleSourceLabel(styleSheetId) {
        const hdr = this.stylesheetHeaders.get(styleSheetId);
        if (!hdr) return 'unknown';
        if (hdr.sourceURL && hdr.sourceURL.length > 0) {
            // Relativize against the page dir so committed IRs are checkout-root independent.
            if (hdr.sourceURL.startsWith(this.baseUrl)) {
                return decodeURIComponent(hdr.sourceURL.slice(this.baseUrl.length));
            }
            try { return decodeURIComponent(new URL(hdr.sourceURL).pathname.split('/').pop()); }
            catch { return hdr.sourceURL; }
        }
        return hdr.isInline ? 'inline <style>' : 'unknown';
    }

    // Author-set properties + provenance, from matched rules (skips UA origin).
    // Also surfaces ::before/::after usage — pseudo-elements paint real pixels the IR
    // cannot represent, so they must be diagnosed (B7).
    async authorProperties(nodeId) {
        let matched;
        try {
            matched = await this.cdp.send('CSS.getMatchedStylesForNode', { nodeId });
        } catch {
            return { props: [], pseudoDiags: [] }; // non-styleable node
        }
        const pseudoDiags = [];
        for (const pm of matched.pseudoElements ?? []) {
            if (pm.pseudoType !== 'before' && pm.pseudoType !== 'after') continue;
            for (const m of pm.matches ?? []) {
                if (m.rule.origin !== 'regular') continue;
                const hdr = this.stylesheetHeaders.get(m.rule.styleSheetId);
                const line = m.rule.style?.range
                    ? m.rule.style.range.startLine + (hdr?.isInline ? hdr.startLine : 0) + 1
                    : null;
                const label = this.ruleSourceLabel(m.rule.styleSheetId);
                pseudoDiags.push({
                    property: `::${pm.pseudoType}`,
                    value: 'pseudo-element content is not representable in the IR',
                    source: line !== null ? `${label}:${line}` : label,
                });
            }
        }
        const out = [];
        const harvest = (style, sourceLabel, lineOffset) => {
            if (!style) return;
            for (const prop of style.cssProperties ?? []) {
                if (prop.disabled) continue;
                // range present => author-written text (not a longhand expansion)
                if (!prop.range && style.range) continue;
                // Inline <style> sheet ranges are sheet-relative; header.startLine rebases
                // them onto the document so the diagnostic points at the real file line.
                const line = prop.range ? prop.range.startLine + lineOffset + 1 : null;
                out.push({ property: prop.name, value: prop.value, source: sourceLabel, line });
            }
        };
        for (const m of matched.matchedCSSRules ?? []) {
            if (m.rule.origin !== 'regular') continue; // UA + injected styles are not author intent
            const hdr = this.stylesheetHeaders.get(m.rule.styleSheetId);
            const label = this.ruleSourceLabel(m.rule.styleSheetId);
            harvest(m.rule.style, label, hdr?.isInline ? hdr.startLine : 0);
        }
        harvest(matched.inlineStyle, 'inline style=', 0);
        return { props: out, pseudoDiags };
    }

    diagnoseUnsupported(authorProps, extraDiags) {
        const seen = new Set();
        const out = [...extraDiags];
        for (const p of authorProps) {
            const name = p.property.toLowerCase();
            if (name.startsWith('--')) continue; // custom properties are inputs, resolved by Chromium
            const nameSupported = SUPPORTED_PROPERTIES.has(name) || SUPPORTED_SHORTHANDS.has(name);
            const valueRule = VALUE_RULES.get(name);
            const valueRejected = valueRule !== undefined && !valueRule.has(p.value.trim());
            if (nameSupported && !valueRejected) continue;
            const key = `${name}|${p.source}|${p.line}`;
            if (seen.has(key)) continue;
            seen.add(key);
            out.push({
                property: name,
                value: p.value,
                source: p.line !== null ? `${p.source}:${p.line}` : p.source,
            });
        }
        out.sort((a, b) => a.property.localeCompare(b.property) || a.source.localeCompare(b.source));
        return out;
    }

    layoutBlock(c, authorProps) {
        const display = c.get('display');
        // Chromium resolves all four inset sides, erasing "which edges did the author pin" —
        // that distinction decides anchors at emission, so recover it from the author rules.
        const authoredSides = new Set();
        // Same intent-erasure for sizing: a used width of 1200 could be an authored `width:1200px`
        // (beats cross-axis stretch) or the stretch result itself. Record authored presence so the
        // layout runtime can reproduce the priority Chromium applied.
        const authoredSizing = new Set();
        const sizingProps = ['width', 'height', 'min-width', 'min-height', 'max-width', 'max-height'];
        for (const p of authorProps) {
            const n = p.property.toLowerCase();
            if (['top', 'right', 'bottom', 'left'].includes(n)) authoredSides.add(n);
            if (n === 'inset') ['top', 'right', 'bottom', 'left'].forEach(s => authoredSides.add(s));
            if (sizingProps.includes(n) && p.value.trim() !== 'auto') authoredSizing.add(n);
            if (n === 'flex' || n === 'flex-basis') authoredSizing.add('basis');
        }
        return {
            display,
            direction: c.get('flex-direction'),
            justify: c.get('justify-content'),
            align: c.get('align-items'),
            alignSelf: c.get('align-self'),
            alignContent: c.get('align-content'),
            wrap: c.get('flex-wrap'),
            gap: [px(c.get('column-gap')), px(c.get('row-gap'))],
            grow: parseFloat(c.get('flex-grow')) || 0,
            shrink: parseFloat(c.get('flex-shrink')) || 0,
            basis: c.get('flex-basis'),
            position: c.get('position'),
            inset: c.get('position') === 'static' ? null : {
                top: c.get('top'), right: c.get('right'),
                bottom: c.get('bottom'), left: c.get('left'),
                authored: [...authoredSides].sort(),
            },
            zIndex: c.get('z-index') === 'auto' ? 0 : parseInt(c.get('z-index'), 10),
            order: parseInt(c.get('order'), 10) || 0,
            boxSizing: c.get('box-sizing'),
            overflow: [c.get('overflow-x'), c.get('overflow-y')],
            sizingAuthored: [...authoredSizing].sort(),
            minSize: [pxOrNull(c.get('min-width')), pxOrNull(c.get('min-height'))],
            maxSize: [pxOrNull(c.get('max-width')), pxOrNull(c.get('max-height'))],
        };
    }

    paintBlock(c, box, unsupportedSink) {
        const bgColor = parseColor(c.get('background-color'));
        const bgImage = c.get('background-image');
        let background = null;
        if (bgImage && bgImage !== 'none') {
            if (bgImage.startsWith('url(')) {
                const url = /^url\("?([^")]+)"?\)$/.exec(bgImage)?.[1];
                background = {
                    type: 'image',
                    asset: url ? this.assetRef(url) : null,
                    size: c.get('background-size'),
                    // background-position shorthand is absent from the computed list; the
                    // -x/-y longhands are authoritative (B2).
                    position: [c.get('background-position-x'), c.get('background-position-y')],
                    repeat: c.get('background-repeat'),
                };
            } else {
                background = {
                    type: 'gradient',
                    ...parseGradient(bgImage, box.border.w, box.border.h),
                    computed: bgImage,
                };
            }
        } else if (bgColor && bgColor[3] !== 0) {
            background = { type: 'color', rgba: bgColor };
        }
        // Computed radii can be "Npx", "N%", or "H V" pairs; % resolves against the border box
        // (horizontal % of width, vertical % of height — B3). Elliptical results (H != V after
        // resolution) are out of the v1 surface and diagnosed, with the horizontal value kept.
        const radius = corner => {
            const raw = c.get(corner);
            const parts = raw.split(' ');
            const resolve = (part, basis) =>
                part.endsWith('%') ? round2(parseFloat(part) / 100 * basis) : px(part);
            const h = resolve(parts[0], box.border.w);
            const v = resolve(parts[1] ?? parts[0], box.border.h);
            if (Math.abs(h - v) > 0.5) {
                unsupportedSink.push({
                    property: corner, value: `${raw} resolves elliptically (${h}px / ${v}px)`,
                    source: 'computed',
                });
            }
            return h;
        };
        // Per-side colors are typed (the consumer bakes bespoke borders); no diagnostic needed.
        const borderColors = ['top', 'right', 'bottom', 'left']
            .map(s => parseColor(c.get(`border-${s}-color`)));
        return {
            background,
            borderRadius: [
                radius('border-top-left-radius'), radius('border-top-right-radius'),
                radius('border-bottom-right-radius'), radius('border-bottom-left-radius'),
            ],
            borderWidth: [
                px(c.get('border-top-width')), px(c.get('border-right-width')),
                px(c.get('border-bottom-width')), px(c.get('border-left-width')),
            ],
            borderColor: borderColors[0],
            borderColors,
            boxShadow: c.get('box-shadow') === 'none' ? null : {
                ...(l => l !== null ? { layers: l } : {})(parseBoxShadow(c.get('box-shadow'))),
                computed: c.get('box-shadow'),
            },
            opacity: round2(parseFloat(c.get('opacity'))),
            transform: c.get('transform') === 'none' ? null : this.transformBlock(c),
            visibility: c.get('visibility'),
        };
    }

    // Chromium computes every transform down to a matrix. 2D matrices become typed
    // [a,b,c,d,tx,ty]; matrix3d stays matrix:null with the verbatim `computed` kept — a
    // consumer that cannot type it must diagnose, not guess. Origin is absolute px (x, y).
    transformBlock(c) {
        const computed = c.get('transform');
        // Matrix coefficients are unitless — 2-decimal rounding would denormalize rotations
        // (0.97²+0.26² ≈ 1.0085 → visible corner error); 6 decimals preserves orthonormality.
        const round6 = n => Math.round(n * 1e6) / 1e6;
        const m = /^matrix\(([^)]+)\)$/.exec(computed);
        const parsed = m ? m[1].split(',').map(v => round6(parseFloat(v))) : null;
        return {
            computed,
            matrix: parsed && parsed.length === 6 && parsed.every(Number.isFinite) ? parsed : null,
            origin: c.get('transform-origin').split(' ').slice(0, 2).map(px),
        };
    }

    textBlock(node, c) {
        // <br> folds to \n (a FORCED break the consumer must honor); whitespace collapses within
        // each segment, not across breaks. Pure-whitespace content stays null.
        const parts = [];
        for (const ch of node.children ?? []) {
            if (ch.nodeType === 3) parts.push(ch.nodeValue);
            else if (ch.nodeType === 1 && ch.nodeName === 'BR') parts.push('\n');
        }
        const content = parts.join('')
            .split('\n').map(s => s.replace(/\s+/g, ' ').trim()).join('\n');
        if (content.replace(/\n/g, '').trim().length === 0) return null;
        // Chrome 150's computed list dropped the `white-space` shorthand in favor of the
        // `white-space-collapse` + `text-wrap-mode` longhands (B1); synthesize the classic value.
        const collapse = c.get('white-space-collapse') ?? 'collapse';
        const wrapMode = c.get('text-wrap-mode') ?? 'wrap';
        const whiteSpace = c.get('white-space')
            ?? { 'collapse|wrap': 'normal', 'collapse|nowrap': 'nowrap',
                 'preserve|nowrap': 'pre', 'preserve|wrap': 'pre-wrap',
                 'preserve-breaks|wrap': 'pre-line' }[`${collapse}|${wrapMode}`]
            ?? `${collapse} ${wrapMode}`;
        return {
            content,
            family: c.get('font-family'),
            sizePx: px(c.get('font-size')),
            weight: parseInt(c.get('font-weight'), 10),
            style: c.get('font-style'),
            lineHeightPx: c.get('line-height') === 'normal' ? null : px(c.get('line-height')),
            letterSpacingPx: c.get('letter-spacing') === 'normal' ? 0 : px(c.get('letter-spacing')),
            color: parseColor(c.get('color')),
            align: c.get('text-align'),
            whiteSpace,
            textOverflow: c.get('text-overflow'),
            transformCase: c.get('text-transform'),
            decoration: c.get('text-decoration-line'),
        };
    }

    ckBlock(attrs, nodeId) {
        for (const key of attrs.keys()) {
            if (key.startsWith('data-ck-') && !KNOWN_CK_ATTRIBUTES.has(key)) {
                this.diagnostics.push({
                    kind: 'unknown-ck-attribute', node: nodeId,
                    detail: `${key}="${attrs.get(key)}" is not a recognized data-ck-* attribute`,
                });
            }
        }
        const name = attrs.get('data-ck-name');
        if (name !== undefined) {
            if (this.ckNames.has(name)) {
                this.diagnostics.push({
                    kind: 'duplicate-ck-name', node: nodeId,
                    detail: `data-ck-name="${name}" already used by ${this.ckNames.get(name)}`,
                });
            } else {
                this.ckNames.set(name, nodeId);
            }
        }
        const has = name !== undefined || attrs.has('data-ck-widget')
            || attrs.has('data-ck-bind') || attrs.has('data-ck-slot');
        if (!has) return null;
        return {
            name: name ?? null,
            widgetClass: attrs.get('data-ck-widget') ?? null,
            bind: attrs.get('data-ck-bind') ?? null,
            slot: attrs.get('data-ck-slot') ?? null,
        };
    }

    // States: force pseudo-class, re-read computed, diff supported properties.
    async stateBlock(nodeId, baseComputed) {
        const states = {};
        for (const pseudo of STATE_PSEUDO_CLASSES) {
            await this.cdp.send('CSS.forcePseudoState', { nodeId, forcedPseudoClasses: [pseudo] });
            const { computedStyle } = await this.cdp.send('CSS.getComputedStyleForNode', { nodeId });
            const forced = this.computedMap(computedStyle);
            const diff = {};
            for (const prop of SUPPORTED_PROPERTIES) {
                const a = baseComputed.get(prop);
                const b = forced.get(prop);
                if (a !== b) diff[prop] = b;
            }
            if (Object.keys(diff).length > 0) states[pseudo] = diff;
        }
        await this.cdp.send('CSS.forcePseudoState', { nodeId, forcedPseudoClasses: [] });
        return Object.keys(states).length > 0 ? states : null;
    }

    async extractElement(node) {
        const attrs = this.attrMap(node);
        if (attrs.has('data-ck-ignore')) return null;

        const { computedStyle } = await this.cdp.send('CSS.getComputedStyleForNode', { nodeId: node.nodeId });
        const c = this.computedMap(computedStyle);
        if (c.get('display') === 'none') return null;

        let box = null;
        try {
            const { model } = await this.cdp.send('DOM.getBoxModel', { nodeId: node.nodeId });
            box = {
                content: quadToRect(model.content),
                padding: quadToRect(model.padding),
                border: quadToRect(model.border),
                margin: quadToRect(model.margin),
            };
        } catch {
            return null; // no box => not rendered
        }

        const text = this.textBlock(node, c);
        if (text !== null) {
            // Line boxes (one rect per rendered line of the folded text) — the comparanda for the
            // sec-8.1 text regime: wrap positions, line origins, advance widths, at CSS-visible
            // granularity.
            try {
                const { object } = await this.cdp.send('DOM.resolveNode', { nodeId: node.nodeId });
                const { result } = await this.cdp.send('Runtime.callFunctionOn', {
                    objectId: object.objectId,
                    returnByValue: true,
                    functionDeclaration: `function() {
                        const r = document.createRange();
                        r.selectNodeContents(this);
                        // Font-fallback splits one visual line into per-run rects (e.g. an em-dash
                        // from a fallback face) — merge rects sharing a line so a lineBox means a
                        // LINE, not a shaping run.
                        const merged = [];
                        for (const b of r.getClientRects()) {
                            const last = merged[merged.length - 1];
                            if (last && Math.abs(b.y - last.y) < Math.max(b.height, last.h) * 0.5) {
                                last.w = Math.max(last.x + last.w, b.x + b.width) - Math.min(last.x, b.x);
                                last.x = Math.min(last.x, b.x);
                                last.h = Math.max(last.h, b.height);
                            } else {
                                merged.push({ x: b.x, y: b.y, w: b.width, h: b.height });
                            }
                        }
                        return merged.map(b =>
                            [b.x, b.y, b.w, b.h].map(v => Math.round(v * 100) / 100));
                    }`,
                });
                if (Array.isArray(result.value)) text.lineBoxes = result.value;
            } catch { /* non-resolvable node: lineBoxes absent, comparator skips */ }
        }

        const { props: authorProps, pseudoDiags } = await this.authorProperties(node.nodeId);
        const id = `n${this.nodeCounter++}`;
        this.domNodeIds.set(id, node.nodeId);
        const imgAsset = node.nodeName === 'IMG' && attrs.has('src')
            ? this.assetRef(attrs.get('src'))
            : null;
        const semanticAttrs = {};
        for (const name of SEMANTIC_ATTRIBUTES) {
            if (attrs.has(name)) semanticAttrs[name] = attrs.get(name) === '' ? true : attrs.get(name);
        }
        const paintSink = [];

        const childElements = [];
        for (const child of node.children ?? []) {
            if (child.nodeType !== 1) continue;
            if (['SCRIPT', 'STYLE', 'LINK', 'META', 'TITLE'].includes(child.nodeName)) continue;
            const extracted = await this.extractElement(child);
            if (extracted) childElements.push(extracted);
        }
        // Yoga has no `order` property: bake the paint/layout order here (stable sort).
        childElements.sort((a, b) => a.layout.order - b.layout.order);

        return {
            id,
            tag: node.nodeName.toLowerCase(),
            ck: this.ckBlock(attrs, id),
            asset: imgAsset,
            attributes: Object.keys(semanticAttrs).length > 0 ? semanticAttrs : null,
            box,
            layout: this.layoutBlock(c, authorProps),
            paint: this.paintBlock(c, box, paintSink),
            text,
            states: await this.stateBlock(node.nodeId, c),
            children: childElements,
            unsupported: this.diagnoseUnsupported(authorProps, [...pseudoDiags, ...paintSink]),
        };
    }
}

// ---------------------------------------------------------------------------------------------

const collectUnsupported = (node, acc = []) => {
    acc.push(...node.unsupported.map(u => ({ ...u, node: node.id })));
    for (const child of node.children) collectUnsupported(child, acc);
    return acc;
};

const main = async () => {
    const inputArg = process.argv[2];
    const outDirArg = process.argv[3];
    if (!inputArg) {
        console.error('usage: node src/extract.mjs <page.html> [outDir]');
        process.exit(2);
    }
    const inputPath = path.resolve(inputArg);
    const outDir = path.resolve(outDirArg ?? path.dirname(inputPath));
    const baseName = path.basename(inputPath, '.html');
    fs.mkdirSync(outDir, { recursive: true });

    const browser = await puppeteer.launch({
        executablePath: findChrome(),
        headless: true,
        args: [
            '--force-color-profile=srgb',
            '--hide-scrollbars',
            '--disable-lcd-text',
            '--force-device-scale-factor=1',
            '--allow-file-access-from-files', // asset normalization canvas-reads file:// images
        ],
    });
    try {
        const page = await browser.newPage();
        await page.setViewport(VIEWPORT);
        await page.goto(pathToFileURL(inputPath).href, { waitUntil: 'networkidle0' });
        await page.evaluate(() => document.fonts.ready);

        const imageMeta = new Map(Object.entries(await page.evaluate(() =>
            Object.fromEntries([...document.images].map(img =>
                [img.src, { naturalWidth: img.naturalWidth, naturalHeight: img.naturalHeight }])))));
        const loadedFonts = await page.evaluate(() =>
            [...document.fonts].map(f => ({ family: f.family, weight: f.weight, style: f.style }))
                .sort((a, b) => a.family.localeCompare(b.family) || a.weight.localeCompare(b.weight)));

        const cdp = await page.createCDPSession();
        const extractor = new Extractor(cdp, pathToFileURL(inputPath).href, imageMeta);
        cdp.on('CSS.styleSheetAdded', e => extractor.stylesheetHeaders.set(e.header.styleSheetId, e.header));
        await cdp.send('DOM.enable');
        await cdp.send('CSS.enable');
        // Neutralize nondeterminism (animations, transitions, caret) via an inspector-origin
        // stylesheet — the diagnostics harvester only reads 'regular'-origin rules, so this
        // injection can never masquerade as author CSS.
        await cdp.send('Page.enable');
        const frameTree = await cdp.send('Page.getFrameTree').catch(() => null);
        const frameId = frameTree?.frameTree?.frame?.id;
        const { styleSheetId: injectedSheet } = await cdp.send('CSS.createStyleSheet', { frameId });
        const NEUTRALIZER =
            '*,*::before,*::after{animation:none !important;transition:none !important;caret-color:transparent !important;}';
        await cdp.send('CSS.setStyleSheetText', { styleSheetId: injectedSheet, text: NEUTRALIZER });

        const root = await extractor.run();
        const version = await browser.version();

        // Second box pass with transforms neutralized: getBoxModel quads are post-transform
        // geometry, but the layout runtime reproduces the UNtransformed rects (the transform is
        // reapplied at paint as a render transform). Nodes whose geometry moves get
        // `boxUntransformed`; the sheet is restored before the golden screenshot, which must
        // keep the transformed pixels.
        const irNodesById = new Map();
        (function walk(n) { irNodesById.set(n.id, n); n.children.forEach(walk); })(root);
        const anyTransform = [...irNodesById.values()].some(n => n.paint.transform !== null);
        if (anyTransform) {
            await cdp.send('CSS.setStyleSheetText', {
                styleSheetId: injectedSheet,
                text: NEUTRALIZER + '*{transform:none !important;}',
            });
            const rectsEqual = (a, b) =>
                ['x', 'y', 'w', 'h'].every(k => Math.abs(a[k] - b[k]) < 0.05);
            for (const [irId, domNodeId] of extractor.domNodeIds) {
                const irNode = irNodesById.get(irId);
                let model;
                try { ({ model } = await cdp.send('DOM.getBoxModel', { nodeId: domNodeId })); }
                catch { continue; }
                const untransformed = {
                    content: quadToRect(model.content),
                    padding: quadToRect(model.padding),
                    border: quadToRect(model.border),
                    margin: quadToRect(model.margin),
                };
                if (!['content', 'padding', 'border', 'margin']
                    .every(k => rectsEqual(untransformed[k], irNode.box[k]))) {
                    irNode.boxUntransformed = untransformed;
                }
            }
            await cdp.send('CSS.setStyleSheetText', { styleSheetId: injectedSheet, text: NEUTRALIZER });
        }

        // Normalize assets through the browser's own decode: Chromium color-manages PNGs (gamma/
        // color chunks), the engine reads raw bytes — the golden is the spec, so the IR bundle
        // ships canvas-decoded copies whose raw bytes ARE the browser's interpretation.
        const assetEntries = [...extractor.assets.values()];
        if (assetEntries.length > 0) {
            fs.mkdirSync(path.join(outDir, 'ckui-assets'), { recursive: true });
            for (const entry of assetEntries) {
                const dataUrl = await page.evaluate(async (src) => {
                    const img = new Image();
                    img.src = src;
                    await img.decode();
                    const canvas = document.createElement('canvas');
                    canvas.width = img.naturalWidth;
                    canvas.height = img.naturalHeight;
                    canvas.getContext('2d').drawImage(img, 0, 0);
                    return canvas.toDataURL('image/png');
                }, entry.resolvedUrl);
                const normalizedRel = `ckui-assets/${entry.id}.png`;
                fs.writeFileSync(path.join(outDir, normalizedRel),
                    Buffer.from(dataUrl.split(',')[1], 'base64'));
                entry.src = normalizedRel; // now relative to the IR file itself
                delete entry.resolvedUrl;
            }
        }

        const ir = {
            schema: SCHEMA_VERSION,
            source: {
                html: path.basename(inputPath),
                viewport: [VIEWPORT.width, VIEWPORT.height],
                dpr: VIEWPORT.deviceScaleFactor,
                browser: version,
            },
            assets: assetEntries,
            fonts: loadedFonts, // web fonts only; system-font stacks live on each node's text.family
            diagnostics: extractor.diagnostics,
            root,
        };

        const jsonPath = path.join(outDir, `${baseName}.ckui.json`);
        fs.writeFileSync(jsonPath, JSON.stringify(ir, null, 2) + '\n');

        const pngPath = path.join(outDir, `${baseName}.golden.png`);
        await page.screenshot({ path: pngPath, clip: { x: 0, y: 0, ...{ width: VIEWPORT.width, height: VIEWPORT.height } } });

        const drops = collectUnsupported(root);
        console.log(`extracted: ${jsonPath}`);
        console.log(`golden:    ${pngPath}`);
        console.log(`browser:   ${version}`);
        if (drops.length > 0) {
            console.log(`\nUNSUPPORTED PROPERTIES (${drops.length}) — will not convert; diagnostics recorded in IR:`);
            for (const d of drops) console.log(`  [${d.node}] ${d.property}: ${d.value}  (${d.source})`);
        } else {
            console.log('unsupported properties: none');
        }
        if (extractor.diagnostics.length > 0) {
            console.log(`\nPAGE DIAGNOSTICS (${extractor.diagnostics.length}):`);
            for (const d of extractor.diagnostics) console.log(`  [${d.node}] ${d.kind}: ${d.detail}`);
        }
    } finally {
        await browser.close();
    }
};

main().catch(e => { console.error(e); process.exit(1); });
