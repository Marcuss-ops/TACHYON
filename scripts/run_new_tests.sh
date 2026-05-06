#!/bin/bash
# Run only the new tests added for schema migration and catalog audit

set -e

BUILD_DIR="/home/pierone/Pyt/Tachyon/build"

echo "Running new schema and catalog tests..."
echo ""

# Run individual tests by creating a custom test list
# Since the test runner runs all tests, we need to use CTest with regex

cd "$BUILD_DIR"

# Run tests with CTest and filter
echo "Running transition_catalog_audit tests..."
ctest -R "transition_catalog_audit" --output-on-failure || true

echo ""
echo "Running layerspec_schema tests..."
ctest -R "layerspec_schema" --output-on-failure || true

echo ""
echo "Running background_catalog_alignment tests..."
ctest -R "background_catalog_alignment" --output-on-failure || true

echo ""
echo "Running missing_transition_fallback tests..."
ctest -R "missing_transition_fallback" --output-on-failure || true

echo ""
echo "Done!"
