cmake_minimum_required(VERSION 3.15)
project(TachyonOperations LANGUAGES CXX)

# --- Subdirectories ---
add_subdirectory(media)
add_subdirectory(decoding)
add_subdirectory(compression)
add_subdirectory(audio)

# --- Compatibility Target ---
# [LEGACY / DEPRECATED]
# TachyonOperations acts as an aggregator target for backward compatibility.
#
# ARCHITECTURAL MANDATE:
#   1. "Core defines contracts. Runtime orchestrates. Backends implement. Operations are thin adapters."
#   2. To prevent circular reference dependencies (TachyonRuntime -> TachyonOperations -> TachyonMediaOps -> TachyonRuntime),
#      we are planning to completely remove this legacy interface target in the next major version.
#   3. New executable / runtime targets should link directly to the specific fine-grained operations libraries 
#      (e.g., TachyonMediaOps, TachyonAudioOps) instead of linking to TachyonOperations.
#
# MILESTONE: Remove TachyonOperations aggregator once CLI/runtime link directly to specific ops targets.
add_library(TachyonOperations INTERFACE)
target_link_libraries(TachyonOperations INTERFACE 
    TachyonMediaOps 
    TachyonDecodingOps
    TachyonAudioOps 
    TachyonCompressionOps
)
