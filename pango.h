#ifndef _WSK_PANGO_H
#define _WSK_PANGO_H
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <cairo/cairo.h>
#include <pango/pangocairo.h>

PangoLayout *get_pango_layout(cairo_t *cairo, const char *font,
		const char *text, double scale);
void get_text_size(cairo_t *cairo, const char *font, int *width, int *height,
		int *baseline, double scale, const char *fmt, ...);
void pango_printf(cairo_t *cairo, const char *font,
		double scale, const char *fmt, ...);

#endif
