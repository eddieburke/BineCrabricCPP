#ifndef STB_VORBIS_INCLUDE_STB_VORBIS_H
#define STB_VORBIS_INCLUDE_STB_VORBIS_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stb_vorbis stb_vorbis;

typedef struct {
   unsigned int sample_rate;
   int channels;
   unsigned int setup_memory_required;
   unsigned int setup_temp_memory_required;
   unsigned int temp_memory_required;
   int max_frame_size;
} stb_vorbis_info;

typedef struct {
   char *alloc_buffer;
   int alloc_buffer_length_in_bytes;
} stb_vorbis_alloc;

stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f);
int stb_vorbis_get_error(stb_vorbis *f);
void stb_vorbis_close(stb_vorbis *f);

stb_vorbis *stb_vorbis_open_memory(const unsigned char *data, int len, int *error, const stb_vorbis_alloc *alloc);
int stb_vorbis_get_frame_float(stb_vorbis *f, int *channels, float ***output);

#ifdef __cplusplus
}
#endif

#endif
