#!/bin/bash
set -e

TACHYON="./build/dev-linux-fast/src/tachyon"
OUT_DIR="output/typewriter"
mkdir -p "$OUT_DIR"

demos=(
    "labs/v2/Typewriter/typewriter_01_basic.cpp"
    "labs/v2/Typewriter/typewriter_02_smooth.cpp"
    "labs/v2/Typewriter/typewriter_03_word.cpp"
    "labs/v2/Typewriter/typewriter_04_blur.cpp"
    "labs/v2/Typewriter/typewriter_05_slide.cpp"
    "labs/v2/Typewriter/typewriter_06_pop.cpp"
    "labs/v2/Typewriter/typewriter_07_fast.cpp"
    "labs/v2/Typewriter/typewriter_08_overlap.cpp"
    "labs/v2/Typewriter/typewriter_09_stagger_blur.cpp"
    "labs/v2/Typewriter/typewriter_10_combo.cpp"
)

for demo in "${demos[@]}"; do
    name=$(basename "$demo" .cpp)
    echo "Rendering typewriter demo: $name"
    # Render only 5 frames to save time and check first frame
    "$TACHYON" render --cpp "$demo" --out "$OUT_DIR/$name.mp4" --frames 5
done

echo "Typewriter rendering complete. Check $OUT_DIR for results."
