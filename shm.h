#ifndef SHM_H
#define SHM_H
#include <cairo/cairo.h>
#include <pango/pangocairo.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int create_shm_file(void);
int allocate_shm_file(size_t size);

struct pool_buffer {
	struct wl_buffer *buffer;
	cairo_surface_t *surface;
	cairo_t *cairo;
	PangoContext *pango;
	uint32_t width, height;
	void *data;
	size_t size;
	bool busy;
};

struct pool_buffer *get_next_buffer(struct wl_shm *shm,
		struct pool_buffer pool[static 2], uint32_t width, uint32_t height);
void destroy_buffer(struct pool_buffer *buffer);

#endif
