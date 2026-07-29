#ifndef MORPH_KITTY_PROTOCOL_H
#define MORPH_KITTY_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MORPH_KITTY_PLACEHOLDER_LIMIT 256u

typedef int (*morph_kitty_write_fn)(const char *bytes, size_t len,
				    void *user_data);

/*
 * Returns a process-unique, randomized image id suitable for Unicode
 * placeholders. The id uses all 32 bits so it does not collide with clients
 * limited to 8-bit or 24-bit placeholder ids.
 */
uint32_t morph_kitty_image_id_new(void);

/*
 * Writes one row of U+10EEEE Unicode placeholders. The image id is encoded in
 * the foreground color and third diacritic as required by Kitty's protocol.
 */
int morph_kitty_write_placeholder_row(morph_kitty_write_fn write,
				      void *user_data,
				      uint32_t image_id,
				      unsigned int row,
				      unsigned int columns);

#ifdef __cplusplus
}
#endif

#endif
