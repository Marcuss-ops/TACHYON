#!/usr/bin/env bash
# Minimal wrapper for the official Linux build script.
# Preferred way: use build.ps1 via pwsh if available.
exec "$(dirname "$0")/scripts/build-linux.sh" "$@"
