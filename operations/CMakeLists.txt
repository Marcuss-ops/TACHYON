cmake_minimum_required(VERSION 3.15)
project(TachyonOperations LANGUAGES CXX)

# --- Subdirectories ---
add_subdirectory(media)
add_subdirectory(assets)
add_subdirectory(decoding)
add_subdirectory(compression)
add_subdirectory(audio)

# --- Compatibility Target ---
# Legacy Interface target for backward compatibility with targets that still link to TachyonOperations
add_library(TachyonOperations INTERFACE)
target_link_libraries(TachyonOperations INTERFACE 
    TachyonMediaOps 
    TachyonAssetOps 
    TachyonDecodingOps
    TachyonAudioOps 
    TachyonCompressionOps
)
