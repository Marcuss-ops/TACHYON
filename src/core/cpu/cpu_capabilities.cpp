#include "tachyon/core/cpu/cpu_capabilities.h"

#include "cpu_features_macros.h"

#if defined(CPU_FEATURES_ARCH_X86)
#include "cpuinfo_x86.h"
#elif defined(CPU_FEATURES_ARCH_ARM)
#include "cpuinfo_arm.h"
#elif defined(CPU_FEATURES_ARCH_AARCH64)
#include "cpuinfo_aarch64.h"
#endif

namespace tachyon::core {

namespace {

CpuCapabilities detect_capabilities() {
    CpuCapabilities caps;

#if defined(CPU_FEATURES_ARCH_X86)
    const auto info = cpu_features::GetX86Info();
    const auto features = info.features;
    
    caps.sse3 = features.sse3;
    caps.sse41 = features.sse41;
    caps.sse42 = features.sse42;
    caps.avx = features.avx;
    caps.avx2 = features.avx2;
    caps.avx512f = features.avx512f;
    
    caps.brand = info.brand;
    caps.family = info.family;
    caps.model = info.model;
    caps.stepping = info.stepping;

#elif defined(CPU_FEATURES_ARCH_ARM)
    const auto info = cpu_features::GetArmInfo();
    const auto features = info.features;
    caps.neon = features.neon;
    caps.brand = "ARM";

#elif defined(CPU_FEATURES_ARCH_AARCH64)
    const auto info = cpu_features::GetAarch64Info();
    const auto features = info.features;
    caps.neon = true; // AArch64 always has NEON-like features
    caps.sve = features.sve;
    caps.brand = "AArch64";
#endif

    return caps;
}

} // namespace

const CpuCapabilities& get_cpu_capabilities() {
    static const CpuCapabilities caps = detect_capabilities();
    return caps;
}

} // namespace tachyon::core
