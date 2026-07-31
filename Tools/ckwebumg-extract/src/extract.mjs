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
const round2 = n => Math.round(n * 100) / 100;

// Quad = [x1,y1,x2,y2,x3,y3,x4,y4] clockwise from top-left. Axis-aligned assumption is
// checked by the caller (transforms make quads non-rectangular; we record both).
const quadToRect = q => ({
    x: round2(Math.min(q[0], q[2], q[4], q[6])),
    y: round2(Math.min(q[1], q[3], q[5], q[7])),
    w: round2(Math.max(q[0], q[2], q[4], q[6]) - Math.min(q[0], q[2], q[4], q[6])),
    h: round2(Math.max(q[1], q[3], q[5], q[7]) - Math.min(q[1], q[3], q[5], q[7])),
});

const parseColor = v => {
    // Chromium computed colors arrive as rgb(r, g, b) / rgba(r, g, b, a). Absolute already.
    const m = /^rgba?\(([\d.]+),\s*([\d.]+),\s*([\d.]+)(?:,\s*([\d.]+))?\)$/.exec(v);
    if (!m) return null;
    return [Number(m[1]), Number(m[2]), Number(m[3]), m[4] === undefined ? 255 : Math.round(Number(m[4]) * 255)];
};

// ---------------------------------------------------------------------------------------------

class Extractor {
    constructor(cdp, pageUrl) {
        this.cdp = cdp;
        this.pageUrl = pageUrl;
        this.nodeCounter = 0;
        this.stylesheetHeaders = new Map(); // styleSheetId -> {sourceURL}
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
            try { return decodeURIComponent(new URL(hdr.sourceURL).pathname.replace(/^\//, '')); }
            catch { return hdr.sourceURL; }
        }
        return hdr.isInline ? 'inline <style>' : 'unknown';
    }

    // Author-set properties + provenance, from matched rules (skips UA origin).
    async authorProperties(nodeId) {
        let matched;
        try {
            matched = await this.cdp.send('CSS.getMatchedStylesForNode', { nodeId });
        } catch {
            return []; // non-styleable node
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
        return out;
    }

    diagnoseUnsupported(authorProps) {
        const seen = new Set();
        const out = [];
        for (const p of authorProps) {
            const name = p.property.toLowerCase();
            if (name.startsWith('--')) continue; // custom properties are inputs, resolved by Chromium
            if (SUPPORTED_PROPERTIES.has(name) || SUPPORTED_SHORTHANDS.has(name)) continue;
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

    layoutBlock(c) {
        const display = c.get('display');
        return {
            display,
            direction: c.get('flex-direction'),
            justify: c.get('justify-content'),
            align: c.get('align-items'),
            alignSelf: c.get('align-self'),
            wrap: c.get('flex-wrap'),
            gap: [px(c.get('column-gap')), px(c.get('row-gap'))],
            grow: parseFloat(c.get('flex-grow')) || 0,
            shrink: parseFloat(c.get('flex-shrink')) || 0,
            basis: c.get('flex-basis'),
            position: c.get('position'),
            inset: c.get('position') === 'static' ? null : {
                top: c.get('top'), right: c.get('right'),
                bottom: c.get('bottom'), left: c.get('left'),
            },
            zIndex: c.get('z-index') === 'auto' ? 0 : parseInt(c.get('z-index'), 10),
            order: parseInt(c.get('order'), 10) || 0,
            boxSizing: c.get('box-sizing'),
            overflow: [c.get('overflow-x'), c.get('overflow-y')],
        };
    }

    paintBlock(c) {
        const bgColor = parseColor(c.get('background-color'));
        const bgImage = c.get('background-image');
        let background = null;
        if (bgImage && bgImage !== 'none') {
            // Gradients and url() are recorded verbatim-computed for Gate 1; the emitter's
            // gradient parser is Gate 3 scope. Recording, not dropping.
            background = { type: bgImage.startsWith('url(') ? 'image' : 'gradient', computed: bgImage };
        } else if (bgColor && bgColor[3] !== 0) {
            background = { type: 'color', rgba: bgColor };
        }
        return {
            background,
            borderRadius: [
                px(c.get('border-top-left-radius')), px(c.get('border-top-right-radius')),
                px(c.get('border-bottom-right-radius')), px(c.get('border-bottom-left-radius')),
            ],
            borderWidth: [
                px(c.get('border-top-width')), px(c.get('border-right-width')),
                px(c.get('border-bottom-width')), px(c.get('border-left-width')),
            ],
            borderColor: parseColor(c.get('border-top-color')),
            boxShadow: c.get('box-shadow') === 'none' ? null : { computed: c.get('box-shadow') },
            opacity: round2(parseFloat(c.get('opacity'))),
            transform: c.get('transform') === 'none' ? null : {
                computed: c.get('transform'), origin: c.get('transform-origin'),
            },
            visibility: c.get('visibility'),
        };
    }

    textBlock(node, c) {
        const textChildren = (node.children ?? []).filter(ch => ch.nodeType === 3);
        const content = textChildren.map(t => t.nodeValue).join('').replace(/\s+/g, ' ').trim();
        if (content.length === 0) return null;
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
            whiteSpace: c.get('white-space'),
            textOverflow: c.get('text-overflow'),
            transformCase: c.get('text-transform'),
            decoration: c.get('text-decoration-line'),
        };
    }

    ckBlock(attrs) {
        const has = attrs.has('data-ck-name') || attrs.has('data-ck-widget')
            || attrs.has('data-ck-bind') || attrs.has('data-ck-slot');
        if (!has) return null;
        return {
            name: attrs.get('data-ck-name') ?? null,
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

        const authorProps = await this.authorProperties(node.nodeId);
        const id = `n${this.nodeCounter++}`;

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
            ck: this.ckBlock(attrs),
            box,
            layout: this.layoutBlock(c),
            paint: this.paintBlock(c),
            text: this.textBlock(node, c),
            states: await this.stateBlock(node.nodeId, c),
            children: childElements,
            unsupported: this.diagnoseUnsupported(authorProps),
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
        ],
    });
    try {
        const page = await browser.newPage();
        await page.setViewport(VIEWPORT);
        await page.goto(pathToFileURL(inputPath).href, { waitUntil: 'networkidle0' });
        await page.evaluate(() => document.fonts.ready);

        const cdp = await page.createCDPSession();
        const extractor = new Extractor(cdp, pathToFileURL(inputPath).href);
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
        await cdp.send('CSS.setStyleSheetText', {
            styleSheetId: injectedSheet,
            text: '*,*::before,*::after{animation:none !important;transition:none !important;caret-color:transparent !important;}',
        });

        const root = await extractor.run();
        const version = await browser.version();

        const ir = {
            schema: SCHEMA_VERSION,
            source: {
                html: path.basename(inputPath),
                viewport: [VIEWPORT.width, VIEWPORT.height],
                dpr: VIEWPORT.deviceScaleFactor,
                browser: version,
            },
            assets: [], // populated when corpus pages reference images (Gate 1 later item)
            fonts: [],  // populated from document.fonts once font mapping lands
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
    } finally {
        await browser.close();
    }
};

main().catch(e => { console.error(e); process.exit(1); });
