#ifndef RADEON_DRM_PUBLIC_H
#define RADEON_DRM_PUBLIC_H

#include "pipe/p_defines.h"

struct radeon_winsys;
struct pipe_screen;
struct pipe_screen_config;

typedef struct pipe_screen *(*radeon_screen_create_t)(struct radeon_winsys *,
                                                      const struct pipe_screen_config *);

struct radeon_winsys *
radeon_drm_winsys_create(int fd, const struct pipe_screen_config *config,
			 radeon_screen_create_t screen_create);

#endif
