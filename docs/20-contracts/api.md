# API

## Purpose

This document defines the native embedding surface for TACHYON.

## Intended use

- render from another C++ application
- integrate into automation tools
- submit jobs without shelling out
- inspect validation and render diagnostics programmatically

## Core API surface

- load a scene spec
- validate a job
- render a composition or frame range
- query diagnostics
- register asset sources
- configure caches and quality tiers

## Design rules

1. API calls should mirror the CLI and render-job model.
2. The API should not expose undefined internal state.
3. Errors should be structured, not string-only.
4. Compatibility should be versioned and documented.
