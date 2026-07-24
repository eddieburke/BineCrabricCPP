#include "stb_vorbis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#endif
#if defined(__linux__) || defined(__sun__) || defined(__EMSCRIPTEN__)
#include <alloca.h>
#endif

#define MAX_CHANNELS 16
#define FAST_HUFFMAN_TABLE_SIZE_LOG2 10
#define FAST_HUFFMAN_TABLE_SIZE (1 << FAST_HUFFMAN_TABLE_SIZE_LOG2)
#define FAST_HUFFMAN_TABLE_MASK (FAST_HUFFMAN_TABLE_SIZE - 1)

typedef float float32;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef int16_t int16;
typedef int32_t int32;

typedef struct {
   int dim;
   int entries;
   uint8 *lens;
   float32 min;
   float32 delta;
   uint8 vbits;
   uint8 ltype;
   uint8 seq;
   uint8 sparse;
   uint32 lvals;
   float32 *mults;
   uint32 *codewords;
   int16 fast_huff[FAST_HUFFMAN_TABLE_SIZE];
   uint32 *sorted_codewords;
   int *sorted_vals;
   int sorted_entries;
} Codebook;

typedef struct {
   uint8 order;
   uint16 rate;
   uint16 bark_map_size;
   uint8 amp_bits;
   uint8 amp_offset;
   uint8 num_books;
   uint8 book_list[16];
} Floor0;

typedef struct {
   uint8 partitions;
   uint8 partition_class_list[32];
   uint8 class_dimensions[16];
   uint8 class_subclasses[16];
   uint8 class_masterbook[16];
   int16 subclass_books[16][8];
   uint16 Xlist[31 * 8 + 2];
   uint8 sorted_order[31 * 8 + 2];
   uint8 neighbors[31 * 8 + 2][2];
   uint8 floor1_multiplier;
   uint8 rangebits;
   int values;
} Floor1;

typedef union {
   Floor0 floor0;
   Floor1 floor1;
} Floor;

typedef struct {
   uint32 begin;
   uint32 end;
   uint32 part_size;
   uint8 classifications;
   uint8 classbook;
   uint8 **classdata;
   int16 (*residue_books)[8];
} Residue;

typedef struct {
   uint8 magnitude;
   uint8 angle;
   uint8 mux;
} MappingChannel;

typedef struct {
   uint16 coupling_steps;
   MappingChannel *chan;
   uint8 submaps;
   uint8 submap_floor[15];
   uint8 submap_residue[15];
} Mapping;

typedef struct {
   uint8 blockflag;
   uint8 mapping;
   uint16 windowtype;
   uint16 transformtype;
} Mode;

typedef struct {
   uint32 goal_crc;
   int bytes_left;
   uint32 crc_so_far;
   int bytes_done;
   uint32 sample_loc;
} CRCscan;

typedef struct {
   uint32 page_start;
   uint32 page_end;
   uint32 last_decoded_sample;
} ProbedPage;

struct stb_vorbis {
   uint32 sample_rate;
   int channels;
   uint32 setup_memory_required;
   uint32 temp_memory_required;
   uint32 setup_temp_memory_required;

   char *vendor;
   int comment_list_length;
   char **comment_list;

   FILE *f;
   uint32 f_start;
   int close_on_free;

   uint8 *stream;
   uint8 *stream_start;
   uint8 *stream_end;
   uint32 stream_len;

   uint8 push_mode;
   uint32 first_audio_page_offset;

   ProbedPage p_first, p_last;
   stb_vorbis_alloc alloc;
   int setup_offset;
   int temp_offset;
   int eof;
   int error;

   int blocksize[2];
   int blocksize_0, blocksize_1;
   int codebook_count;
   Codebook *codebooks;
   int floor_count;
   uint16 floor_types[64];
   Floor *floor_config;
   int residue_count;
   uint16 residue_types[64];
   Residue *residue_config;
   int mapping_count;
   Mapping *mapping;
   int mode_count;
   Mode mode_config[64];

   uint32 total_samples;
   float32 *channel_buffers[MAX_CHANNELS];
   float32 *outputs[MAX_CHANNELS];
   float32 *previous_window[MAX_CHANNELS];
   int previous_length;
   int16 *finalY[MAX_CHANNELS];

   uint32 current_loc;
   int current_loc_valid;

   float32 *A[2], *B[2], *C[2], *window[2];
   uint16 *bit_reverse[2];

   uint32 serial;
   int last_page;
   int segment_count;
   uint8 segments[255];
   uint8 page_flag;
   uint8 bytes_in_seg;
   uint8 first_decode;
   int next_seg;
   int last_seg;
   int last_seg_which;
   uint32 acc;
   int valid_bits;
   int packet_bytes;
   int end_seg_with_known_loc;
   uint32 known_loc_for_packet;
   int discard_samples_deferred;
   uint32 samples_output;
   int page_crc_tests;
   CRCscan scan[4];
   int channel_buffer_start;
   int channel_buffer_end;
};

enum STBVorbisError {
   VORBIS__no_error,
   VORBIS_need_more_data = 1,
   VORBIS_invalid_api_mixing,
   VORBIS_outofmem,
   VORBIS_feature_not_supported,
   VORBIS_too_many_channels,
   VORBIS_file_open_failure,
   VORBIS_seek_without_length,
   VORBIS_unexpected_eof = 10,
   VORBIS_seek_invalid,
   VORBIS_invalid_setup = 20,
   VORBIS_invalid_stream,
   VORBIS_missing_capture_pattern = 30,
   VORBIS_invalid_stream_structure_version,
   VORBIS_continued_packet_flag_invalid,
   VORBIS_incorrect_stream_serial_number,
   VORBIS_invalid_first_page,
   VORBIS_bad_packet_type,
   VORBIS_cant_find_last_page,
   VORBIS_seek_failed,
   VORBIS_ogg_skeleton_not_supported
};

static int error(stb_vorbis *f, enum STBVorbisError e) {
   f->error = e;
   return 0;
}

static void *setup_malloc(stb_vorbis *f, int sz) {
   sz = (sz + 7) & ~7;
   f->setup_memory_required += sz;
   if (f->alloc.alloc_buffer) {
      void *p = (char *) f->alloc.alloc_buffer + f->setup_offset;
      if (f->setup_offset + sz > f->temp_offset) return NULL;
      f->setup_offset += sz;
      return p;
   }
   return sz ? malloc(sz) : NULL;
}

static void setup_free(stb_vorbis *f, void *p) {
   if (!f->alloc.alloc_buffer) free(p);
}

static void *temp_malloc(stb_vorbis *f, int sz) {
   sz = (sz + 7) & ~7;
   if (f->alloc.alloc_buffer) {
      if (f->temp_offset - sz < f->setup_offset) return NULL;
      f->temp_offset -= sz;
      return (char *) f->alloc.alloc_buffer + f->temp_offset;
   }
   return malloc(sz);
}

static void temp_free(stb_vorbis *f, void *p, int sz) {
   if (f->alloc.alloc_buffer) f->temp_offset += (sz + 7) & ~7;
   else free(p);
}

#define temp_alloc(f,sz) (f->alloc.alloc_buffer ? temp_malloc(f,sz) : alloca(sz))
#define temp_alloc_save(f) ((f)->temp_offset)
#define temp_alloc_restore(f,p) ((f)->temp_offset = (p))

static uint32 crc_table[256];
static void crc_init(void) {
   for (int i = 0; i < 256; ++i) {
      uint32 s = (uint32) i << 24;
      for (int j = 0; j < 8; ++j) s = (s << 1) ^ (s >= (1U << 31) ? 0x04c11db7 : 0);
      crc_table[i] = s;
   }
}

static uint32 bit_reverse(uint32 n) {
   n = ((n & 0xAAAAAAAA) >> 1) | ((n & 0x55555555) << 1);
   n = ((n & 0xCCCCCCCC) >> 2) | ((n & 0x33333333) << 2);
   n = ((n & 0xF0F0F0F0) >> 4) | ((n & 0x0F0F0F0F) << 4);
   n = ((n & 0xFF00FF00) >> 8) | ((n & 0x00FF00FF) << 8);
   return (n >> 16) | (n << 16);
}

static int ilog(int32 n) {
   static signed char log2_4[16] = {0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
   if (n < 0) return 0;
   if (n < (1 << 14)) return n < (1 << 4) ? log2_4[n] : (n < (1 << 9) ? 5 + log2_4[n >> 5] : 10 + log2_4[n >> 10]);
   return n < (1 << 24) ? (n < (1 << 19) ? 15 + log2_4[n >> 15] : 20 + log2_4[n >> 20]) : (n < (1 << 29) ? 25 + log2_4[n >> 25] : 30 + log2_4[n >> 30]);
}

static float32 float32_unpack(uint32 x) {
   uint32 mantissa = x & 0x1fffff;
   uint32 sign = x & 0x80000000;
   uint32 exponent = (x & 0x7fe00000) >> 21;
   return (float32) ldexp(sign ? -(double)mantissa : (double)mantissa, (int)exponent - 788);
}

static void add_entry(Codebook *c, uint32 huff_code, int symbol, int count, int len, uint32 *values) {
   if (!c->sparse) {
      c->codewords[symbol] = huff_code;
   } else {
      c->codewords[count] = huff_code;
      c->lens[count] = (uint8) len;
      values[count] = symbol;
   }
}

static int compute_codewords(Codebook *c, uint8 *lengths, int n, uint32 *values) {
   int k, m = 0;
   uint32 available[32] = {0};
   for (k = 0; k < n; ++k) if (lengths[k] < 255) break;
   if (k == n) return 1;
   add_entry(c, 0, k, m++, lengths[k], values);
   for (int i = 1; i <= lengths[k]; ++i) available[i] = 1U << (32 - i);
   for (int i = k + 1; i < n; ++i) {
      int z = lengths[i];
      if (z == 255) continue;
      while (z > 0 && !available[z]) --z;
      if (!z) return 0;
      uint32 res = available[z];
      available[z] = 0;
      add_entry(c, bit_reverse(res), i, m++, lengths[i], values);
      if (z != lengths[i]) {
         for (int y = lengths[i]; y > z; --y) available[y] = res + (1 << (32 - y));
      }
   }
   return 1;
}

static void compute_fast_huffman(Codebook *c) {
   for (int i = 0; i < FAST_HUFFMAN_TABLE_SIZE; ++i) c->fast_huff[i] = -1;
   int len = c->sparse ? c->sorted_entries : c->entries;
   if (len > 32767) len = 32767;
   for (int i = 0; i < len; ++i) {
      if (c->lens[i] <= FAST_HUFFMAN_TABLE_SIZE_LOG2) {
         uint32 z = c->sparse ? bit_reverse(c->sorted_codewords[i]) : c->codewords[i];
         while (z < FAST_HUFFMAN_TABLE_SIZE) {
            c->fast_huff[z] = (int16) i;
            z += 1 << c->lens[i];
         }
      }
   }
}

static int uint32_compare(const void *p, const void *q) {
   uint32 x = *(uint32 *) p, y = *(uint32 *) q;
   return x < y ? -1 : x > y;
}

static void compute_sorted_huffman(Codebook *c, uint8 *lengths, uint32 *values) {
   int len = c->sparse ? c->sorted_entries : c->entries;
   if (!c->sparse) {
      int k = 0;
      for (int i = 0; i < c->entries; ++i) {
         if (lengths[i] != 255 && lengths[i] > FAST_HUFFMAN_TABLE_SIZE_LOG2)
            c->sorted_codewords[k++] = bit_reverse(c->codewords[i]);
      }
   } else {
      for (int i = 0; i < c->sorted_entries; ++i)
         c->sorted_codewords[i] = bit_reverse(c->codewords[i]);
   }
   qsort(c->sorted_codewords, c->sorted_entries, sizeof(uint32), uint32_compare);
   c->sorted_codewords[c->sorted_entries] = 0xffffffff;
   for (int i = 0; i < len; ++i) {
      int hlen = c->sparse ? lengths[values[i]] : lengths[i];
      if (c->sparse || (hlen != 255 && hlen > FAST_HUFFMAN_TABLE_SIZE_LOG2)) {
         uint32 code = bit_reverse(c->codewords[i]);
         int x = 0, n = c->sorted_entries;
         while (n > 1) {
            int m = x + (n >> 1);
            if (c->sorted_codewords[m] <= code) { x = m; n -= (n >> 1); }
            else n >>= 1;
         }
         if (c->sparse) {
            c->sorted_vals[x] = values[i];
            c->lens[x] = (uint8) hlen;
         } else {
            c->sorted_vals[x] = i;
         }
      }
   }
}

static uint8 get8(stb_vorbis *z) {
   if (z->stream) {
      if (z->stream >= z->stream_end) { z->eof = 1; return 0; }
      return *z->stream++;
   }
   int c = fgetc(z->f);
   if (c == EOF) { z->eof = 1; return 0; }
   return (uint8) c;
}

static uint32 get32(stb_vorbis *f) {
   uint32 x = get8(f);
   x += get8(f) << 8;
   x += get8(f) << 16;
   return x + ((uint32) get8(f) << 24);
}

static int getn(stb_vorbis *z, uint8 *data, int n) {
   if (z->stream) {
      if (z->stream + n > z->stream_end) { z->eof = 1; return 0; }
      memcpy(data, z->stream, n);
      z->stream += n;
      return 1;
   }
   return fread(data, n, 1, z->f) == 1 ? 1 : (z->eof = 1, 0);
}

static void skip(stb_vorbis *z, int n) {
   if (z->stream) {
      z->stream += n;
      if (z->stream >= z->stream_end) z->eof = 1;
   } else {
      fseek(z->f, ftell(z->f) + n, SEEK_SET);
   }
}

static uint8 ogg_header[4] = {0x4f, 0x67, 0x67, 0x53};
static int capture_pattern(stb_vorbis *f) {
   for (int i = 0; i < 4; ++i) if (ogg_header[i] != get8(f)) return 0;
   return 1;
}

static int start_page_no_capturepattern(stb_vorbis *f) {
   if (0 != get8(f)) return error(f, VORBIS_invalid_stream_structure_version);
   f->page_flag = get8(f);
   uint32 l0 = get32(f), l1 = get32(f);
   get32(f);
   uint32 n = get32(f);
   f->last_page = n;
   get32(f);
   f->segment_count = get8(f);
   if (!getn(f, f->segments, f->segment_count)) return error(f, VORBIS_unexpected_eof);
   f->end_seg_with_known_loc = -2;
   if (l0 != ~0U || l1 != ~0U) {
      for (int i = f->segment_count - 1; i >= 0; --i) {
         if (f->segments[i] < 255) {
            f->end_seg_with_known_loc = i;
            f->known_loc_for_packet = l0;
            break;
         }
      }
   }
   f->next_seg = 0;
   return 1;
}

static int start_page(stb_vorbis *f) {
   return capture_pattern(f) ? start_page_no_capturepattern(f) : error(f, VORBIS_missing_capture_pattern);
}

static int start_packet(stb_vorbis *f) {
   while (f->next_seg == -1) {
      if (!start_page(f)) return 0;
      if (f->page_flag & 1) return error(f, VORBIS_continued_packet_flag_invalid);
   }
   f->last_seg = f->valid_bits = f->packet_bytes = f->bytes_in_seg = 0;
   return 1;
}

static int next_segment(stb_vorbis *f) {
   if (f->last_seg) return 0;
   if (f->next_seg == -1) {
      f->last_seg_which = f->segment_count - 1;
      if (!start_page(f)) { f->last_seg = 1; return 0; }
      if (!(f->page_flag & 1)) return error(f, VORBIS_continued_packet_flag_invalid);
   }
   int len = f->segments[f->next_seg++];
   if (len < 255) { f->last_seg = 1; f->last_seg_which = f->next_seg - 1; }
   if (f->next_seg >= f->segment_count) f->next_seg = -1;
   f->bytes_in_seg = (uint8) len;
   return len;
}

static int get8_packet_raw(stb_vorbis *f) {
   if (!f->bytes_in_seg) {
      if (f->last_seg || !next_segment(f)) return -1;
   }
   --f->bytes_in_seg;
   ++f->packet_bytes;
   return get8(f);
}

static int get8_packet(stb_vorbis *f) {
   int x = get8_packet_raw(f);
   f->valid_bits = 0;
   return x;
}

static uint32 get32_packet(stb_vorbis *f) {
   uint32 x = get8_packet(f);
   x += get8_packet(f) << 8;
   x += get8_packet(f) << 16;
   return x + ((uint32) get8_packet(f) << 24);
}

static void flush_packet(stb_vorbis *f) {
   while (get8_packet_raw(f) != -1);
}

static uint32 get_bits(stb_vorbis *f, int n) {
   if (f->valid_bits < 0) return 0;
   if (f->valid_bits < n) {
      if (n > 24) {
         uint32 z = get_bits(f, 24);
         return z + (get_bits(f, n - 24) << 24);
      }
      if (!f->valid_bits) f->acc = 0;
      while (f->valid_bits < n) {
         int k = get8_packet_raw(f);
         if (k == -1) { f->valid_bits = -1; return 0; }
         f->acc += (uint32) k << f->valid_bits;
         f->valid_bits += 8;
      }
   }
   uint32 z = f->acc & ((1 << n) - 1);
   f->acc >>= n;
   f->valid_bits -= n;
   return z;
}

static void prep_huffman(stb_vorbis *f) {
   if (f->valid_bits <= 24) {
      if (!f->valid_bits) f->acc = 0;
      do {
         if (f->last_seg && !f->bytes_in_seg) return;
         int z = get8_packet_raw(f);
         if (z == -1) return;
         f->acc += (uint32) z << f->valid_bits;
         f->valid_bits += 8;
      } while (f->valid_bits <= 24);
   }
}

static int codebook_decode_scalar_raw(stb_vorbis *f, Codebook *c) {
   prep_huffman(f);
   if (!c->codewords && !c->sorted_codewords) return -1;
   if (c->entries > 8 ? c->sorted_codewords != NULL : !c->codewords) {
      uint32 code = bit_reverse(f->acc);
      int x = 0, n = c->sorted_entries;
      while (n > 1) {
         int m = x + (n >> 1);
         if (c->sorted_codewords[m] <= code) { x = m; n -= (n >> 1); }
         else n >>= 1;
      }
      if (!c->sparse) x = c->sorted_vals[x];
      int len = c->lens[x];
      if (f->valid_bits >= len) {
         f->acc >>= len;
         f->valid_bits -= len;
         return x;
      }
      f->valid_bits = 0;
      return -1;
   }
   for (int i = 0; i < c->entries; ++i) {
      if (c->lens[i] == 255) continue;
      if (c->codewords[i] == (f->acc & ((1 << c->lens[i]) - 1))) {
         if (f->valid_bits >= c->lens[i]) {
            f->acc >>= c->lens[i];
            f->valid_bits -= c->lens[i];
            return i;
         }
         f->valid_bits = 0;
         return -1;
      }
   }
   error(f, VORBIS_invalid_stream);
   f->valid_bits = 0;
   return -1;
}

#define DECODE_RAW(v,f,c) \
   if (f->valid_bits < FAST_HUFFMAN_TABLE_SIZE_LOG2) prep_huffman(f); \
   v = f->acc & FAST_HUFFMAN_TABLE_MASK; \
   v = c->fast_huff[v]; \
   if (v >= 0) { \
      int n = c->lens[v]; \
      f->acc >>= n; \
      f->valid_bits -= n; \
      if (f->valid_bits < 0) { f->valid_bits = 0; v = -1; } \
   } else v = codebook_decode_scalar_raw(f,c);

#define DECODE(v,f,c) DECODE_RAW(v,f,c) if (c->sparse) v = c->sorted_vals[v];

static int vorbis_validate(uint8 *p) {
   return memcmp(p, "vorbis", 6) == 0;
}

static int vorbis_decode_initial(stb_vorbis *f) {
   uint8 header[6];
   if (!start_page(f)) return 0;
   if (!(f->page_flag & 2)) return error(f, VORBIS_invalid_first_page);
   if (!start_packet(f)) return 0;

   int packet_type = get8_packet(f);
   if (packet_type != 1) return error(f, VORBIS_invalid_first_page);
   if (!getn(f, header, 6) || !vorbis_validate(header)) return error(f, VORBIS_invalid_first_page);
   if (get32_packet(f) != 0) return error(f, VORBIS_invalid_first_page);

   f->channels = get8_packet(f);
   if (!f->channels || f->channels > MAX_CHANNELS) return error(f, VORBIS_too_many_channels);
   f->sample_rate = get32_packet(f);
   get32_packet(f); get32_packet(f); get32_packet(f);

   int log2_0 = get8_packet(f);
   f->blocksize_0 = 1 << (log2_0 & 15);
   f->blocksize_1 = 1 << (log2_0 >> 4);
   if (f->blocksize_0 < 64 || f->blocksize_1 < f->blocksize_0 || f->blocksize_1 > 8192)
      return error(f, VORBIS_invalid_first_page);
   if (get8_packet(f) != 1) return error(f, VORBIS_invalid_first_page);
   flush_packet(f);

   if (!start_packet(f)) return 0;
   packet_type = get8_packet(f);
   if (packet_type != 3) return error(f, VORBIS_invalid_setup);
   if (!getn(f, header, 6) || !vorbis_validate(header)) return error(f, VORBIS_invalid_setup);
   uint32 len = get32_packet(f);
   skip(f, len);
   int num_comments = get32_packet(f);
   for (int i = 0; i < num_comments; ++i) {
      len = get32_packet(f);
      skip(f, len);
   }
   if (get8_packet(f) != 1) return error(f, VORBIS_invalid_setup);
   flush_packet(f);

   if (!start_packet(f)) return 0;
   packet_type = get8_packet(f);
   if (packet_type != 5) return error(f, VORBIS_invalid_setup);
   if (!getn(f, header, 6) || !vorbis_validate(header)) return error(f, VORBIS_invalid_setup);

   f->codebook_count = get8_packet(f) + 1;
   f->codebooks = (Codebook *) setup_malloc(f, sizeof(Codebook) * f->codebook_count);
   memset(f->codebooks, 0, sizeof(Codebook) * f->codebook_count);

   for (int i = 0; i < f->codebook_count; ++i) {
      Codebook *c = f->codebooks + i;
      if (get8_packet(f) != 0x56 || get8_packet(f) != 0x43 || get8_packet(f) != 0x42)
         return error(f, VORBIS_invalid_setup);
      c->dim = (get8_packet(f) << 8) | get8_packet(f);
      c->entries = get8_packet(f) | (get8_packet(f) << 8) | (get8_packet(f) << 16);
      int ordered = get_bits(f, 1);
      c->lens = (uint8 *) setup_malloc(f, c->entries);
      if (!ordered) {
         int sparse = get_bits(f, 1);
         c->sparse = (uint8) sparse;
         int total = 0;
         for (int j = 0; j < c->entries; ++j) {
            if (sparse && !get_bits(f, 1)) {
               c->lens[j] = 255;
            } else {
               c->lens[j] = (uint8) (get_bits(f, 5) + 1);
               ++total;
            }
         }
         c->sorted_entries = sparse ? total : c->entries;
      } else {
         c->sparse = 0;
         c->sorted_entries = c->entries;
         int current_entry = 0;
         int current_length = get_bits(f, 5) + 1;
         while (current_entry < c->entries) {
            int num = get_bits(f, ilog(c->entries - current_entry));
            for (int j = 0; j < num; ++j) c->lens[current_entry + j] = (uint8) current_length;
            current_entry += num;
            ++current_length;
         }
      }
      int lookup_type = get_bits(f, 4);
      c->ltype = (uint8) lookup_type;
      if (lookup_type > 2) return error(f, VORBIS_invalid_setup);
      if (lookup_type > 0) {
         c->min = float32_unpack(get32_packet(f));
         c->delta = float32_unpack(get32_packet(f));
         c->vbits = (uint8) (get_bits(f, 4) + 1);
         c->seq = (uint8) get_bits(f, 1);
         int quantvals = (lookup_type == 1) ? (int) floor(pow((float)c->entries, 1.0f / c->dim)) : c->entries * c->dim;
         c->mults = (float32 *) setup_malloc(f, sizeof(float32) * quantvals);
         for (int j = 0; j < quantvals; ++j) c->mults[j] = (float32) get_bits(f, c->vbits);
      }
      c->codewords = (uint32 *) setup_malloc(f, sizeof(uint32) * c->sorted_entries);
      if (c->sparse) {
         uint32 *values = (uint32 *) setup_malloc(f, sizeof(uint32) * c->sorted_entries);
         compute_codewords(c, c->lens, c->entries, values);
         c->sorted_codewords = (uint32 *) setup_malloc(f, sizeof(uint32) * (c->sorted_entries + 1));
         c->sorted_vals = (int *) setup_malloc(f, sizeof(int) * c->sorted_entries);
         compute_sorted_huffman(c, c->lens, values);
         setup_free(f, values);
      } else {
         compute_codewords(c, c->lens, c->entries, NULL);
         compute_fast_huffman(c);
      }
   }

   int time_count = get_bits(f, 6) + 1;
   for (int i = 0; i < time_count; ++i) if (get_bits(f, 16) != 0) return error(f, VORBIS_invalid_setup);

   f->floor_count = get_bits(f, 6) + 1;
   f->floor_config = (Floor *) setup_malloc(f, sizeof(Floor) * f->floor_count);
   for (int i = 0; i < f->floor_count; ++i) {
      f->floor_types[i] = (uint16) get_bits(f, 16);
      if (f->floor_types[i] == 1) {
         Floor1 *g = &f->floor_config[i].floor1;
         g->partitions = (uint8) get_bits(f, 5);
         int max_class = -1;
         for (int j = 0; j < g->partitions; ++j) {
            g->partition_class_list[j] = (uint8) get_bits(f, 4);
            if (g->partition_class_list[j] > max_class) max_class = g->partition_class_list[j];
         }
         for (int j = 0; j <= max_class; ++j) {
            g->class_dimensions[j] = (uint8) (get_bits(f, 3) + 1);
            g->class_subclasses[j] = (uint8) get_bits(f, 2);
            if (g->class_subclasses[j]) g->class_masterbook[j] = (uint8) get_bits(f, 8);
            for (int k = 0; k < (1 << g->class_subclasses[j]); ++k)
               g->subclass_books[j][k] = (int16) ((int) get_bits(f, 8) - 1);
         }
         g->floor1_multiplier = (uint8) (get_bits(f, 2) + 1);
         g->rangebits = (uint8) get_bits(f, 4);
         int floor1_values = 2;
         g->Xlist[0] = 0;
         g->Xlist[1] = 1 << g->rangebits;
         for (int j = 0; j < g->partitions; ++j) {
            int c = g->partition_class_list[j];
            for (int k = 0; k < g->class_dimensions[c]; ++k)
               g->Xlist[floor1_values++] = (uint16) get_bits(f, g->rangebits);
         }
         g->values = floor1_values;
         for (int j = 0; j < floor1_values; ++j) g->sorted_order[j] = (uint8) j;
         for (int j = 0; j < floor1_values; ++j) {
            for (int k = j + 1; k < floor1_values; ++k) {
               if (g->Xlist[g->sorted_order[j]] > g->Xlist[g->sorted_order[k]]) {
                  uint8 tmp = g->sorted_order[j];
                  g->sorted_order[j] = g->sorted_order[k];
                  g->sorted_order[k] = tmp;
               }
            }
         }
         for (int j = 2; j < floor1_values; ++j) {
            int low = 0, high = 1;
            for (int k = 0; k < j; ++k) {
               if (g->Xlist[k] < g->Xlist[j] && g->Xlist[k] > g->Xlist[low]) low = k;
               if (g->Xlist[k] > g->Xlist[j] && g->Xlist[k] < g->Xlist[high]) high = k;
            }
            g->neighbors[j][0] = (uint8) low;
            g->neighbors[j][1] = (uint8) high;
         }
      } else return error(f, VORBIS_invalid_setup);
   }

   f->residue_count = get_bits(f, 6) + 1;
   f->residue_config = (Residue *) setup_malloc(f, sizeof(Residue) * f->residue_count);
   for (int i = 0; i < f->residue_count; ++i) {
      uint16 rtype = (uint16) get_bits(f, 16);
      f->residue_types[i] = rtype;
      if (rtype > 2) return error(f, VORBIS_invalid_setup);
      Residue *r = f->residue_config + i;
      r->begin = get_bits(f, 24);
      r->end = get_bits(f, 24);
      r->part_size = get_bits(f, 24) + 1;
      r->classifications = (uint8) (get_bits(f, 6) + 1);
      r->classbook = (uint8) get_bits(f, 8);
      int max_class = r->classifications;
      uint8 *cascade = (uint8 *) setup_malloc(f, max_class);
      for (int j = 0; j < max_class; ++j) {
         int high_bits = 0, low_bits = get_bits(f, 3);
         if (get_bits(f, 1)) high_bits = get_bits(f, 5);
         cascade[j] = (uint8) (high_bits * 8 + low_bits);
      }
      r->res_books = (int16 (*)[8]) setup_malloc(f, sizeof(int16) * 8 * max_class);
      for (int j = 0; j < max_class; ++j) {
         for (int k = 0; k < 8; ++k) {
            if (cascade[j] & (1 << k)) r->res_books[j][k] = (int16) get_bits(f, 8);
            else r->res_books[j][k] = -1;
         }
      }
      setup_free(f, cascade);
      int class_entries = f->codebooks[r->classbook].entries;
      r->classdata = (uint8 **) setup_malloc(f, sizeof(uint8 *) * class_entries);
      int dim = f->codebooks[r->classbook].dim;
      for (int j = 0; j < class_entries; ++j) {
         r->classdata[j] = (uint8 *) setup_malloc(f, dim);
         int temp = j;
         for (int k = dim - 1; k >= 0; --k) {
            r->classdata[j][k] = (uint8) (temp % r->classifications);
            temp /= r->classifications;
         }
      }
   }

   f->mapping_count = get_bits(f, 6) + 1;
   f->mapping = (Mapping *) setup_malloc(f, sizeof(Mapping) * f->mapping_count);
   for (int i = 0; i < f->mapping_count; ++i) {
      if (get_bits(f, 16) != 0) return error(f, VORBIS_invalid_setup);
      Mapping *m = f->mapping + i;
      int submaps = get_bits(f, 1);
      m->submaps = submaps ? (uint8) (get_bits(f, 4) + 1) : 1;
      if (get_bits(f, 1)) {
         m->coupling_steps = (uint16) (get_bits(f, 8) + 1);
         m->chan = (MappingChannel *) setup_malloc(f, sizeof(MappingChannel) * m->coupling_steps);
         int n = ilog(f->channels - 1);
         for (int j = 0; j < m->coupling_steps; ++j) {
            m->chan[j].magnitude = (uint8) get_bits(f, n);
            m->chan[j].angle = (uint8) get_bits(f, n);
         }
      } else m->coupling_steps = 0;
      if (get_bits(f, 2) != 0) return error(f, VORBIS_invalid_setup);
      if (m->submaps > 1) {
         for (int j = 0; j < f->channels; ++j) m->chan[j].mux = (uint8) get_bits(f, 4);
      } else {
         for (int j = 0; j < f->channels; ++j) m->chan[j].mux = 0;
      }
      for (int j = 0; j < m->submaps; ++j) {
         get_bits(f, 8);
         m->submap_floor[j] = (uint8) get_bits(f, 8);
         m->submap_residue[j] = (uint8) get_bits(f, 8);
      }
   }

   f->mode_count = get_bits(f, 6) + 1;
   for (int i = 0; i < f->mode_count; ++i) {
      Mode *m = f->mode_config + i;
      m->blockflag = (uint8) get_bits(f, 1);
      m->windowtype = (uint16) get_bits(f, 16);
      m->transformtype = (uint16) get_bits(f, 16);
      m->mapping = (uint8) get_bits(f, 8);
   }
   if (get_bits(f, 1) != 1) return error(f, VORBIS_invalid_setup);

   f->blocksize[0] = f->blocksize_0;
   f->blocksize[1] = f->blocksize_1;
   for (int i = 0; i < f->channels; ++i) {
      f->channel_buffers[i] = (float32 *) setup_malloc(f, sizeof(float32) * f->blocksize_1);
      f->previous_window[i] = (float32 *) setup_malloc(f, sizeof(float32) * (f->blocksize_1 >> 1));
      f->finalY[i] = (int16 *) setup_malloc(f, sizeof(int16) * (31 * 8 + 2));
   }
   return 1;
}

stb_vorbis *stb_vorbis_open_memory(const uint8 *data, int len, int *error_code, const stb_vorbis_alloc *alloc) {
   if (!data) { if (error_code) *error_code = VORBIS_unexpected_eof; return NULL; }
   stb_vorbis p;
   memset(&p, 0, sizeof(p));
   if (alloc) {
      p.alloc = *alloc;
      p.alloc.alloc_buffer_length_in_bytes &= ~7;
      p.temp_offset = p.alloc.alloc_buffer_length_in_bytes;
   }
   p.stream = (uint8 *) data;
   p.stream_end = (uint8 *) data + len;
   p.stream_start = (uint8 *) p.stream;
   p.stream_len = len;
   crc_init();
   p.first_decode = 1;
   if (!vorbis_decode_initial(&p)) {
      if (error_code) *error_code = p.error;
      return NULL;
   }
   stb_vorbis *f = (stb_vorbis *) setup_malloc(&p, sizeof(*f));
   if (f) {
      *f = p;
      if (error_code) *error_code = 0;
      return f;
   }
   return NULL;
}

stb_vorbis_info stb_vorbis_get_info(stb_vorbis *f) {
   stb_vorbis_info d;
   d.channels = f->channels;
   d.sample_rate = f->sample_rate;
   d.setup_memory_required = f->setup_memory_required;
   d.setup_temp_memory_required = f->setup_temp_memory_required;
   d.temp_memory_required = f->temp_memory_required;
   d.max_frame_size = f->blocksize_1 >> 1;
   return d;
}

int stb_vorbis_get_error(stb_vorbis *f) {
   int e = f->error;
   f->error = 0;
   return e;
}

void stb_vorbis_close(stb_vorbis *p) {
   if (!p) return;
   if (!p->alloc.alloc_buffer) {
      if (p->vendor) free(p->vendor);
      if (p->codebooks) free(p->codebooks);
      if (p->floor_config) free(p->floor_config);
      if (p->residue_config) free(p->residue_config);
      if (p->mapping) free(p->mapping);
      for (int i = 0; i < p->channels; ++i) {
         if (p->channel_buffers[i]) free(p->channel_buffers[i]);
         if (p->previous_window[i]) free(p->previous_window[i]);
         if (p->finalY[i]) free(p->finalY[i]);
      }
      free(p);
   }
}

int stb_vorbis_get_frame_float(stb_vorbis *f, int *channels, float ***output) {
   if (!f) return 0;
   if (channels) *channels = f->channels;
   if (output) *output = f->outputs;
   return 0;
}
