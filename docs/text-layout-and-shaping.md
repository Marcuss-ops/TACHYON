# Text Layout and Shaping

## Purpose

This document defines the minimum text engine expectations for TACHYON.

## Why text matters

Programmatic video systems depend heavily on text.
If text layout is weak, the engine cannot support serious template-driven motion design.

## Core responsibilities

The text system should define:

- font loading
- fallback behavior
- shaping
- kerning and tracking
- line breaking
- paragraph layout
- text box constraints
- baseline rules
- per-glyph addressability for animation
- glyph metrics
- wrapping and alignment
- text-on-path layout
- per-run and per-word selection boundaries

## Minimum layout contract

The engine should keep layout results explicit enough to answer:

- what glyph is placed at what position
- which font and fallback resolved it
- how line breaks were chosen
- what transforms apply to each run
- what the baseline and advance metrics were

## Shaping rules

- shaping should happen before animation
- animation should operate on resolved runs or glyphs, not raw Unicode text
- font fallback must be deterministic for the same inputs
- text-on-path should preserve the same logical ordering guarantees where possible

## Determinism requirement

The same text input, font set, and layout rules must produce the same layout result across runs in the same engine version.

## First implementation target

The first implementation does not need every advanced typography feature.
It does need a documented layout model good enough for titles, lower thirds, callouts, captions, and data-driven text templates.

## Animation relevance

The text system must integrate cleanly with the property and animation systems so text can be animated at object, line, word, and glyph-related levels over time.
