#ifndef CPVC_VIDEO_POLICY_H
#define CPVC_VIDEO_POLICY_H
#include "cpmap_projection_preferences.h"
enum cpvc_video_mode { CPVC_VIDEO_OEM=0, CPVC_VIDEO_MAP=1, CPVC_VIDEO_MIRROR=2 };
static int cpvc_video_requested(const struct cpmap_projection_preferences *p,
                                int valid, int authorized, int enabled)
{
    if (!valid || !authorized || !enabled) return CPVC_VIDEO_OEM;
    if (p->map_mask & 1) return CPVC_VIDEO_MAP;
    if (p->mirror_mask & 1) return CPVC_VIDEO_MIRROR;
    return CPVC_VIDEO_OEM;
}
#endif
