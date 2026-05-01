# TachyonAudio sources
set(TachyonAudioSources ${TACHYON_ALL_CPP})
list(FILTER TachyonAudioSources INCLUDE REGEX "/audio/[^/]+\\.cpp$")
