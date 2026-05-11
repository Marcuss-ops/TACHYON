#pragma once

#ifdef TACHYON_ENABLE_TRACY
#include <tracy/Tracy.hpp>

#define TACHYON_ZONE(name) ZoneScopedN(name)
#define TACHYON_ZONE_F ZoneScoped
#define TACHYON_FRAME_MARK FrameMark
#else
#define TACHYON_ZONE(name)
#define TACHYON_ZONE_F
#define TACHYON_FRAME_MARK
#endif
