#ifndef MORPH_MARKDOWN_NATIVE_H
#define MORPH_MARKDOWN_NATIVE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct morph_ios_engine morph_ios_engine;

typedef struct morph_ios_bitmap {
	uint32_t width;
	uint32_t height;
	uint32_t *pixels_rgba;
} morph_ios_bitmap;

morph_ios_engine *morph_ios_engine_create(void);
int morph_ios_engine_append(morph_ios_engine *engine,
			    const char *bytes,
			    size_t len,
			    int final);
char *morph_ios_engine_snapshot_json(morph_ios_engine *engine);
void morph_ios_engine_destroy(morph_ios_engine *engine);

morph_ios_bitmap *morph_ios_render_latex(const char *font_path,
					 const char *latex,
					 int display,
					 double font_size);
void morph_ios_bitmap_destroy(morph_ios_bitmap *bitmap);
void morph_ios_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
