# Testing and Compatibility

## Purpose

This document defines the testing and version-compatibility goals for TACHYON.

## Why it matters

A deterministic rendering engine becomes infrastructure only when its behavior can be verified and preserved.

## Test categories

The project should eventually include at least:

- spec validation tests
- property evaluation tests
- timeline and time-mapping tests
- determinism tests
- render graph tests
- visual regression tests
- text layout tests
- caching and invalidation tests

## Visual regression

Golden scenes and golden frames should become part of the core validation story.
Tolerance policy should be documented rather than improvised per test.

## Compatibility

The engine should version:

- scene spec expectations
- render-job expectations
- behavior changes that alter output

## Rule

Changes that intentionally alter visible output should be treated as compatibility-significant changes and documented explicitly.

## Goal

Testing should not be an afterthought added after the renderer exists.
It should be designed as part of the engine contract from the beginning.
