#!/usr/bin/env python3
"""Audit a coremetrics --debug-layout JSON dump for UI issues.

Reads one or more JSON files emitted by `coremetrics --debug-layout PATH`
and checks them against a set of layout invariants. Exits non-zero if any
critical finding is reported, so the script can gate CI on visual regressions
without needing a screenshot diff baseline.

Checks performed:
  * out-of-bounds  text rects extend past the canvas dimensions
  * text-vs-text   two distinct text labels overlap on screen
  * footer-spill   text ink crosses a footer separator rule
  * empty-text     zero-width / zero-height text rect (renderer no-op smell)
  * right-margin   text within RIGHT_MARGIN_PX of the right edge (warning)

Usage:
    layout-audit.py system.json processes.json about.json
"""

from __future__ import annotations

import json
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable


RIGHT_MARGIN_PX = 4
SEVERITY_FAIL = "FAIL"
SEVERITY_WARN = "WARN"

# TTF_RenderText_Blended surfaces include ascender + descender padding,
# so a 20pt body face rasterizes to ~27px tall even though cap-height
# glyphs only occupy roughly y+4..y+21 and descenders reach y+24. We
# crop the surface bbox by these per-side paddings before running the
# intersection test so dead surface padding does not flag vertically
# adjacent labels, while genuine ink collisions still fail. The Body
# bottom pad is 3, not the descender-free 5 an earlier revision used:
# that looser crop hid real collisions (the NET sparkline's "100" and
# "0" ticks overlapped by 9px and still audited clean).
TEXT_PAD_TOP = {"Caption": 2, "Body": 4, "Title": 5}
TEXT_PAD_BOTTOM = {"Caption": 3, "Body": 3, "Title": 4}

# Footer separator detection for the footer-spill check: a near-full-
# width, hairline-thin box in the bottom band of the canvas (the
# y=478 rule above the footer chrome in base.xml). Card borders are
# also thin full-width boxes, so the check is restricted to the
# footer band rather than applied to every rule on screen.
SEPARATOR_MIN_W = 900
SEPARATOR_MAX_H = 4
FOOTER_BAND_PX = 70


@dataclass
class Rect:
    kind: str
    x: int
    y: int
    w: int
    h: int
    text: str
    size: str

    @property
    def x2(self) -> int:
        return self.x + self.w

    @property
    def y2(self) -> int:
        return self.y + self.h

    def intersects(self, other: "Rect") -> bool:
        return not (
            self.x2 <= other.x
            or other.x2 <= self.x
            or self.y2 <= other.y
            or other.y2 <= self.y
        )

    def visible_bbox(self) -> tuple[int, int, int, int]:
        if self.kind != "text":
            return (self.x, self.y, self.x2, self.y2)
        pad_top = TEXT_PAD_TOP.get(self.size, 4)
        pad_bot = TEXT_PAD_BOTTOM.get(self.size, 5)
        return (self.x, self.y + pad_top, self.x2, self.y2 - pad_bot)

    def visible_intersects(self, other: "Rect") -> bool:
        ax1, ay1, ax2, ay2 = self.visible_bbox()
        bx1, by1, bx2, by2 = other.visible_bbox()
        return not (ax2 <= bx1 or bx2 <= ax1 or ay2 <= by1 or by2 <= ay1)


@dataclass
class Finding:
    severity: str
    rule: str
    message: str


def load_rects(payload: dict) -> list[Rect]:
    rects: list[Rect] = []
    for r in payload.get("rects", []):
        rects.append(
            Rect(
                kind=r.get("kind", ""),
                x=int(r.get("x", 0)),
                y=int(r.get("y", 0)),
                w=int(r.get("w", 0)),
                h=int(r.get("h", 0)),
                text=str(r.get("text", "")),
                size=str(r.get("size", "")),
            )
        )
    return rects


def check_out_of_bounds(
    rects: Iterable[Rect], canvas_w: int, canvas_h: int
) -> list[Finding]:
    out: list[Finding] = []
    for r in rects:
        if r.kind != "text":
            continue
        if r.x < 0 or r.y < 0 or r.x2 > canvas_w or r.y2 > canvas_h:
            out.append(
                Finding(
                    SEVERITY_FAIL,
                    "out-of-bounds",
                    f"text '{r.text}' at ({r.x},{r.y}) size {r.w}x{r.h} extends past canvas {canvas_w}x{canvas_h}",
                )
            )
    return out


def check_empty_text(rects: Iterable[Rect]) -> list[Finding]:
    out: list[Finding] = []
    for r in rects:
        if r.kind != "text":
            continue
        if r.w <= 0 or r.h <= 0:
            out.append(
                Finding(
                    SEVERITY_WARN,
                    "empty-text",
                    f"text '{r.text}' has zero dimensions at ({r.x},{r.y})",
                )
            )
    return out


def check_text_overlap(rects: list[Rect]) -> list[Finding]:
    texts = [r for r in rects if r.kind == "text" and r.w > 0 and r.h > 0]
    out: list[Finding] = []
    for i in range(len(texts)):
        a = texts[i]
        for j in range(i + 1, len(texts)):
            b = texts[j]
            if a.text == b.text and a.x == b.x and a.y == b.y:
                continue
            if not a.visible_intersects(b):
                continue
            out.append(
                Finding(
                    SEVERITY_FAIL,
                    "text-overlap",
                    f"'{a.text}' @({a.x},{a.y}) {a.w}x{a.h} overlaps '{b.text}' @({b.x},{b.y}) {b.w}x{b.h}",
                )
            )
    return out


def check_footer_spill(
    rects: list[Rect], canvas_h: int
) -> list[Finding]:
    separators = [
        r
        for r in rects
        if r.kind == "box"
        and r.w >= SEPARATOR_MIN_W
        and r.h <= SEPARATOR_MAX_H
        and r.y >= canvas_h - FOOTER_BAND_PX
    ]
    out: list[Finding] = []
    for r in rects:
        if r.kind != "text":
            continue
        _, iy1, _, iy2 = r.visible_bbox()
        for sep in separators:
            # Text that lives entirely below the rule is footer chrome;
            # only ink that starts above the rule and reaches into or
            # past it has spilled out of the content area.
            if iy1 < sep.y and iy2 > sep.y:
                out.append(
                    Finding(
                        SEVERITY_FAIL,
                        "footer-spill",
                        f"text '{r.text}' at ({r.x},{r.y}) size {r.w}x{r.h} crosses the footer rule at y={sep.y}",
                    )
                )
                break
    return out


def check_right_margin(
    rects: Iterable[Rect], canvas_w: int
) -> list[Finding]:
    out: list[Finding] = []
    threshold = canvas_w - RIGHT_MARGIN_PX
    for r in rects:
        if r.kind != "text":
            continue
        if r.x2 > threshold and r.x2 <= canvas_w:
            out.append(
                Finding(
                    SEVERITY_WARN,
                    "right-margin",
                    f"text '{r.text}' ends at x={r.x2}, within {RIGHT_MARGIN_PX}px of canvas edge {canvas_w}",
                )
            )
    return out


def audit_file(path: Path) -> tuple[str, list[Finding]]:
    with path.open() as f:
        payload = json.load(f)
    canvas = payload.get("canvas", {})
    cw = int(canvas.get("w", 0))
    ch = int(canvas.get("h", 0))
    tab = payload.get("tab", path.stem)
    rects = load_rects(payload)
    findings: list[Finding] = []
    findings.extend(check_out_of_bounds(rects, cw, ch))
    findings.extend(check_empty_text(rects))
    findings.extend(check_text_overlap(rects))
    findings.extend(check_footer_spill(rects, ch))
    findings.extend(check_right_margin(rects, cw))
    return tab, findings


def main() -> int:
    if len(sys.argv) < 2:
        print("usage: layout-audit.py FILE [FILE ...]", file=sys.stderr)
        return 2

    paths = [Path(p) for p in sys.argv[1:]]
    total_fail = 0
    total_warn = 0
    for path in paths:
        if not path.exists():
            print(f"[ERROR] {path}: not found", file=sys.stderr)
            total_fail += 1
            continue
        tab, findings = audit_file(path)
        fails = [f for f in findings if f.severity == SEVERITY_FAIL]
        warns = [f for f in findings if f.severity == SEVERITY_WARN]
        total_fail += len(fails)
        total_warn += len(warns)
        header = f"tab '{tab}' ({path.name}): {len(fails)} fail, {len(warns)} warn"
        if findings:
            print(header)
            for f in findings:
                print(f"  [{f.severity}] {f.rule}: {f.message}")
        else:
            print(header + " (clean)")
    print()
    print(f"TOTAL: {total_fail} fail, {total_warn} warn")
    return 1 if total_fail > 0 else 0


if __name__ == "__main__":
    sys.exit(main())
