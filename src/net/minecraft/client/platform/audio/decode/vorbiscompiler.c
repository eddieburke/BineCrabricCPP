/* Single translation unit for the entire native audio stack:
 *   1. OGG Vorbis decoder  (vc_* API, clean-room)
 *   2. caudio engine       (device I/O + mixer + voices; declared in engine/caudio.h)
 *   3. audio backend       (named-source slot table + C ABI; declared in backend/audio_backend.h)
 * Compiled directly as the audio TU (formerly reached via the audio_backend.c shim). */
#define VORBISCOMPILER_NO_S16   /* engine consumes f32 only; drop the s16 conversion helpers */

#ifndef VORBISCOMPILER_H
#define VORBISCOMPILER_H
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct{char*alloc_buffer;int alloc_buffer_length_in_bytes;}vc_alloc;
typedef struct vc_stream vc_stream;
typedef struct{unsigned int sample_rate;int channels;unsigned int setup_memory_required,setup_temp_memory_required,temp_memory_required;int max_frame_size;}vc_stream_info;
typedef struct{char*vendor;int comment_list_length;char**comment_list;}vc_comment;
extern vc_stream_info vc_get_info(vc_stream*f);
extern vc_comment vc_get_comment(vc_stream*f);
extern int vc_get_error(vc_stream*f);
extern void vc_close(vc_stream*f);
extern int vc_get_sample_offset(vc_stream*f);
extern unsigned int vc_get_file_offset(vc_stream*f);
#ifndef VORBISCOMPILER_NO_PUSH
extern vc_stream*vc_open_pushdata(const unsigned char*datablock,int datablock_length_in_bytes,int*datablock_memory_consumed_in_bytes,int*error,const vc_alloc*alloc_buffer);
extern int vc_decode_frame_pushdata(vc_stream*f,const unsigned char*datablock,int datablock_length_in_bytes,int*channels,float***output,int*samples);
extern void vc_flush_pushdata(vc_stream*f);
#endif
#ifndef VORBISCOMPILER_NO_PULL
#if !defined(VORBISCOMPILER_NO_STDIO)&&!defined(VORBISCOMPILER_NO_S16)
extern int vc_decode_filename(const char*filename,int*channels,int*sample_rate,short**output);
#endif
#if !defined(VORBISCOMPILER_NO_S16)
extern int vc_decode_memory(const unsigned char*mem,int len,int*channels,int*sample_rate,short**output);
#endif
extern vc_stream*vc_open_memory(const unsigned char*data,int len,int*error,const vc_alloc*alloc_buffer);
#ifndef VORBISCOMPILER_NO_STDIO
extern vc_stream*vc_open_filename(const char*filename,int*error,const vc_alloc*alloc_buffer);
extern vc_stream*vc_open_file(FILE*f,int close_handle_on_close,int*error,const vc_alloc*alloc_buffer);
extern vc_stream*vc_open_file_section(FILE*f,int close_handle_on_close,int*error,const vc_alloc*alloc_buffer,unsigned int len);
#endif
extern int vc_seek_frame(vc_stream*f,unsigned int sample_number);
extern int vc_seek(vc_stream*f,unsigned int sample_number);
extern int vc_seek_start(vc_stream*f);
extern unsigned int vc_stream_length_samples(vc_stream*f);
extern float vc_stream_length_seconds(vc_stream*f);
extern int vc_get_frame_float(vc_stream*f,int*channels,float***output);
#ifndef VORBISCOMPILER_NO_S16
extern int vc_get_frame_s16_interleaved(vc_stream*f,int num_c,short*buffer,int num_shorts);
extern int vc_get_frame_s16(vc_stream*f,int num_c,short**buffer,int num_samples);
#endif
extern int vc_read_samples_f32_interleaved(vc_stream*f,int channels,float*buffer,int num_floats);
extern int vc_read_samples_f32(vc_stream*f,int channels,float**buffer,int num_samples);
#ifndef VORBISCOMPILER_NO_S16
extern int vc_read_samples_s16_interleaved(vc_stream*f,int channels,short*buffer,int num_shorts);
extern int vc_read_samples_s16(vc_stream*f,int channels,short**buffer,int num_samples);
#endif
#endif
enum vc_status{VC_OK,VC_NEED_DATA=1,VC_BAD_API,VC_OOM,VC_UNSUPPORTED,VC_TOO_MANY_CH,VC_FILE_OPEN,VC_SEEK_NO_LEN,VC_EOF=10,VC_SEEK_BAD,VC_BAD_SETUP=20,VC_BAD_STREAM,VC_NO_CAPTURE=30,VC_BAD_VER,VC_BAD_PKT_FLAG,VC_BAD_SERIAL,VC_BAD_FIRST_PAGE,VC_BAD_PKT,VC_NO_LAST_PAGE,VC_SEEK_FAIL,VC_OGG_SKEL};
#ifdef __cplusplus
}
#endif
#endif
#ifndef VORBISCOMPILER_HEADER_ONLY
#ifndef VORBISCOMPILER_MAX_CH
#define VORBISCOMPILER_MAX_CH 16
#endif
#ifndef VORBISCOMPILER_CRC_N
#define VORBISCOMPILER_CRC_N 4
#endif
#ifndef VORBISCOMPILER_HUFF_BITS
#define VORBISCOMPILER_HUFF_BITS 10
#endif
#ifndef VORBISCOMPILER_HUFF_INT
#define VORBISCOMPILER_HUFF_S16
#endif
#ifdef VORBISCOMPILER_CODEBOOK_SHORTS
#error "VORBISCOMPILER_CODEBOOK_SHORTS is no longer supported as it produced incorrect results for some input formats"
#endif
#ifdef VORBISCOMPILER_NO_PULL
#define VORBISCOMPILER_NO_S16
#define VORBISCOMPILER_NO_STDIO
#endif
#if defined(VORBISCOMPILER_NO_CRT) && !defined(VORBISCOMPILER_NO_STDIO)
#define VORBISCOMPILER_NO_STDIO 1
#endif
#ifndef VORBISCOMPILER_NO_S16
#ifndef VORBISCOMPILER_NO_FAST_SCL
#ifndef VORBISCOMPILER_BIG_ENDIAN
#define VORBISCOMPILER_ENDIAN 0
#else
#define VORBISCOMPILER_ENDIAN 1
#endif
#endif
#endif
#ifndef VORBISCOMPILER_NO_STDIO
#include <stdio.h>
#endif
#ifndef VORBISCOMPILER_NO_CRT
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#endif
#if defined(__linux__) || defined(__linux) || defined(__sun__) || defined(__EMSCRIPTEN__) || defined(__NEWLIB__)
#include <alloca.h>
#endif
#else
#define NULL 0
#define malloc(s) 0
#define free(s) ((void) 0)
#define realloc(s) 0
#endif
#include <limits.h>
#ifdef __MINGW32__
#ifdef __forceinline
#undef __forceinline
#endif
#define __forceinline
#ifndef alloca
#define alloca __builtin_alloca
#endif
#elif !defined(_MSC_VER)
#if __GNUC__
#define __forceinline inline
#else
#define __forceinline
#endif
#endif
#if VORBISCOMPILER_MAX_CH > 256
#error "Value of VORBISCOMPILER_MAX_CH outside of allowed range"
#endif
#if VORBISCOMPILER_HUFF_BITS > 24
#error "Value of VORBISCOMPILER_HUFF_BITS outside of allowed range"
#endif
#if 0
#include <crtdbg.h>
#define CHECK(f) _CrtIsValidHeapPointer(f->channel_buffers[1])
#else
#define CHECK(f) ((void) 0)
#endif
#define _VC_BLK_LOG 13
#define _VC_BLK (1 << _VC_BLK_LOG)
typedef unsigned char uint8;
typedef signed char int8;
typedef unsigned short uint16;
typedef signed short int16;
typedef unsigned int uint32;
typedef signed int int32;
#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
typedef float _VcCt;
#ifdef _MSC_VER
#define VC_UNUSED(v) (void)(v)
#else
#define VC_UNUSED(v) (void)sizeof(v)
#endif
#define _VC_HUFF_SZ (1 << VORBISCOMPILER_HUFF_BITS)
#define _VC_HUFF_MSK (_VC_HUFF_SZ - 1)
typedef struct { int dimensions, entries; uint8 *codeword_lengths; float minimum_value; float delta_value; uint8 value_bits; uint8 lookup_type; uint8 sequence_p; uint8 sparse; uint32 lookup_values; _VcCt *multiplicands; uint32 *codewords;
#ifdef VORBISCOMPILER_HUFF_S16
int16 fast_huffman[_VC_HUFF_SZ];
#else
int32 fast_huffman[_VC_HUFF_SZ];
#endif
uint32 *sorted_codewords; int *sorted_values; int sorted_entries; }
_VcCb;
typedef struct { uint8 order; uint16 rate; uint16 bark_map_size; uint8 amplitude_bits; uint8 amplitude_offset; uint8 number_of_books; uint8 book_list[16]; }
_VcF0;
typedef struct { uint8 partitions; uint8 partition_class_list[32]; uint8 class_dimensions[16]; uint8 class_subclasses[16]; uint8 class_masterbooks[16]; int16 subclass_books[16][8]; uint16 Xlist[31*8+2]; uint8 sorted_order[31*8+2]; uint8 neighbors[31*8+2][2]; uint8 floor1_multiplier; uint8 rangebits; int values; }
_VcF1;
typedef union { _VcF0 floor0; _VcF1 floor1; }
_VcFl;
typedef struct { uint32 begin, end; uint32 part_size; uint8 classifications; uint8 classbook; uint8 **classdata; int16 (*residue_books)[8]; }
_VcRs;
typedef struct { uint8 magnitude; uint8 angle; uint8 mux; }
_VcMc;
typedef struct { uint16 coupling_steps; _VcMc *chan; uint8 submaps; uint8 submap_floor[15]; uint8 submap_residue[15]; }
_VcMp;
typedef struct { uint8 blockflag; uint8 mapping; uint16 windowtype; uint16 transformtype; }
_VcMd;
typedef struct { uint32 goal_crc; int bytes_left; uint32 crc_so_far; int bytes_done; uint32 sample_loc; }
_VcCrc;
typedef struct { uint32 page_start, page_end; uint32 last_decoded_sample; }
_VcPg;
struct vc_stream { unsigned int sample_rate; int channels; unsigned int setup_memory_required; unsigned int temp_memory_required; unsigned int setup_temp_memory_required; char *vendor; int comment_list_length; char **comment_list;
#ifndef VORBISCOMPILER_NO_STDIO
FILE *f; uint32 f_start; int close_on_free;
#endif
uint8 *stream; uint8 *stream_start; uint8 *stream_end; uint32 stream_len; uint8 push_mode; uint32 first_audio_page_offset; _VcPg p_first, p_last; vc_alloc alloc; int setup_offset; int temp_offset; int eof; enum vc_status error; int blocksize[2]; int blocksize_0, blocksize_1; int codebook_count; _VcCb *codebooks; int floor_count; uint16 floor_types[64]; _VcFl *floor_config; int residue_count; uint16 residue_types[64]; _VcRs *residue_config; int mapping_count; _VcMp *mapping; int mode_count; _VcMd mode_config[64]; uint32 total_samples; float *channel_buffers[VORBISCOMPILER_MAX_CH]; float *outputs [VORBISCOMPILER_MAX_CH]; float *previous_window[VORBISCOMPILER_MAX_CH]; int previous_length;
#ifndef VORBISCOMPILER_NO_DEFER_FLR
int16 *finalY[VORBISCOMPILER_MAX_CH];
#else
float *floor_buffers[VORBISCOMPILER_MAX_CH];
#endif
uint32 current_loc; int current_loc_valid; float *A[2],*B[2],*C[2]; float *window[2]; uint16 *bit_reverse[2]; uint32 serial; int last_page; int segment_count; uint8 segments[255]; uint8 page_flag; uint8 bytes_in_seg; uint8 first_decode; int next_seg; int last_seg; int last_seg_which; uint32 acc; int valid_bits; int packet_bytes; int end_seg_with_known_loc; uint32 known_loc_for_packet; int discard_samples_deferred; uint32 samples_output; int page_crc_tests;
#ifndef VORBISCOMPILER_NO_PUSH
_VcCrc scan[VORBISCOMPILER_CRC_N];
#endif
int channel_buffer_start; int channel_buffer_end; }
;
#if defined(VORBISCOMPILER_NO_PUSH)
#define _VC_PUSH(f) FALSE
#elif defined(VORBISCOMPILER_NO_PULL)
#define _VC_PUSH(f) TRUE
#else
#define _VC_PUSH(f) ((f)->push_mode)
#endif
typedef struct vc_stream _Vcs;
static int _vc_11f9578d(_Vcs *f, enum vc_status e) { f->error = e; if (!f->eof && e != VC_NEED_DATA) { f->error=e; } return 0; }
#define array_size_required(count,size) (count*(sizeof(void *)+(size)))
#define temp_alloc(f,size) (f->alloc.alloc_buffer ? setup_temp_malloc(f,size) : alloca(size))
#define temp_free(f,p) (void)0
#define temp_alloc_save(f) ((f)->temp_offset)
#define temp_alloc_restore(f,p) ((f)->temp_offset = (p))
#define temp_block_array(f,count,size) make_block_array(temp_alloc(f,array_size_required(count,size)), count, size)
static void *make_block_array(void *mem, int count, int size) { int i; void ** p = (void **) mem; char *q = (char *) (p + count); for (i=0; i < count; ++i) { p[i] = q; q += size; } return p; }
static void *setup_malloc(_Vcs *f, int sz) { sz = (sz+7) & ~7; f->setup_memory_required += sz; if (f->alloc.alloc_buffer) { void *p = (char *) f->alloc.alloc_buffer + f->setup_offset; if (f->setup_offset + sz > f->temp_offset) return NULL; f->setup_offset += sz; return p; } return sz ? malloc(sz) : NULL; }
static void _vc_ce046650(_Vcs *f, void *p) { if (f->alloc.alloc_buffer) return; free(p); }
static void *setup_temp_malloc(_Vcs *f, int sz) { sz = (sz+7) & ~7; if (f->alloc.alloc_buffer) { if (f->temp_offset - sz < f->setup_offset) return NULL; f->temp_offset -= sz; return (char *) f->alloc.alloc_buffer + f->temp_offset; } return malloc(sz); }
static void _vc_8c8cdbe9(_Vcs *f, void *p, int sz) { if (f->alloc.alloc_buffer) { f->temp_offset += (sz+7)&~7; return; } free(p); }
#define _VC_CRC_POLY 0x04c11db7
static uint32 crc_table[256];
static void _vc_b66b74bf(void) { int i,j; uint32 s; for(i=0; i < 256; i++) { for (s=(uint32) i << 24, j=0; j < 8; ++j) s = (s << 1) ^ (s >= (1U<<31) ? _VC_CRC_POLY : 0); crc_table[i] = s; } }
static __forceinline uint32 _vc_a1939352(uint32 crc, uint8 byte) { return (crc << 8) ^ crc_table[byte ^ (crc >> 24)]; }
static unsigned int _vc_7f6d13b2(unsigned int n) { n = ((n & 0xAAAAAAAA) >> 1) | ((n & 0x55555555) << 1); n = ((n & 0xCCCCCCCC) >> 2) | ((n & 0x33333333) << 2); n = ((n & 0xF0F0F0F0) >> 4) | ((n & 0x0F0F0F0F) << 4); n = ((n & 0xFF00FF00) >> 8) | ((n & 0x00FF00FF) << 8); return (n >> 16) | (n << 16); }
static float _vc_bfed89e8(float x) { return x*x; }
static int _vc_98fda42c(int32 n) { static signed char log2_4[16] = { 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4 }; if (n < 0) return 0; if (n < (1 << 14)) if (n < (1 << 4)) return 0 + log2_4[n ]; else if (n < (1 << 9)) return 5 + log2_4[n >> 5]; else return 10 + log2_4[n >> 10]; else if (n < (1 << 24)) if (n < (1 << 19)) return 15 + log2_4[n >> 15]; else return 20 + log2_4[n >> 20]; else if (n < (1 << 29)) return 25 + log2_4[n >> 25]; else return 30 + log2_4[n >> 30]; }
#ifndef M_PI
#define M_PI 3.14159265358979323846264f
#endif
#define _VC_NC 255
static float _vc_3fd60b95(uint32 x) { uint32 mantissa = x & 0x1fffff; uint32 sign = x & 0x80000000; uint32 exp = (x & 0x7fe00000) >> 21; double res = sign ? -(double)mantissa : (double)mantissa; return (float) ldexp((float)res, (int)exp-788); }
static void _vc_ddaa4ca2(_VcCb *c, uint32 huff_code, int symbol, int count, int len, uint32 *values) { if (!c->sparse) { c->codewords [symbol] = huff_code; } else { c->codewords [count] = huff_code; c->codeword_lengths[count] = len; values [count] = symbol; } }
static int _vc_561d1c9e(_VcCb *c, uint8 *len, int n, uint32 *values) { int i,k,m=0; uint32 available[32]; memset(available, 0, sizeof(available)); for (k=0; k < n; ++k) if (len[k] < _VC_NC) break; if (k == n) { assert(c->sorted_entries == 0); return TRUE; } assert(len[k] < 32); _vc_ddaa4ca2(c, 0, k, m++, len[k], values); for (i=1; i <= len[k]; ++i) available[i] = 1U << (32-i); for (i=k+1; i < n; ++i) { uint32 res; int z = len[i], y; if (z == _VC_NC) continue; assert(z < 32); while (z > 0 && !available[z]) --z; if (z == 0) { return FALSE; } res = available[z]; available[z] = 0; _vc_ddaa4ca2(c, _vc_7f6d13b2(res), i, m++, len[i], values); if (z != len[i]) { for (y=len[i]; y > z; --y) { assert(available[y] == 0); available[y] = res + (1 << (32-y)); } } } return TRUE; }
static void _vc_6c87b769(_VcCb *c) { int i, len; for (i=0; i < _VC_HUFF_SZ; ++i) c->fast_huffman[i] = -1; len = c->sparse ? c->sorted_entries : c->entries;
#ifdef VORBISCOMPILER_HUFF_S16
if (len > 32767) len = 32767;
#endif
for (i=0; i < len; ++i) { if (c->codeword_lengths[i] <= VORBISCOMPILER_HUFF_BITS) { uint32 z = c->sparse ? _vc_7f6d13b2(c->sorted_codewords[i]) : c->codewords[i]; while (z < _VC_HUFF_SZ) { c->fast_huffman[z] = i; z += 1 << c->codeword_lengths[i]; } } } }
#ifdef _MSC_VER
#define VC_CDECL __cdecl
#else
#define VC_CDECL
#endif
static int VC_CDECL _vc_b90b777a(const void *p, const void *q) { uint32 x = * (uint32 *) p; uint32 y = * (uint32 *) q; return x < y ? -1 : x > y; }
static int _vc_9894107b(_VcCb *c, uint8 len) { if (c->sparse) { assert(len != _VC_NC); return TRUE; } if (len == _VC_NC) return FALSE; if (len > VORBISCOMPILER_HUFF_BITS) return TRUE; return FALSE; }
static void _vc_b27db06e(_VcCb *c, uint8 *lengths, uint32 *values) { int i, len; if (!c->sparse) { int k = 0; for (i=0; i < c->entries; ++i) if (_vc_9894107b(c, lengths[i])) c->sorted_codewords[k++] = _vc_7f6d13b2(c->codewords[i]); assert(k == c->sorted_entries); } else { for (i=0; i < c->sorted_entries; ++i) c->sorted_codewords[i] = _vc_7f6d13b2(c->codewords[i]); } qsort(c->sorted_codewords, c->sorted_entries, sizeof(c->sorted_codewords[0]), _vc_b90b777a); c->sorted_codewords[c->sorted_entries] = 0xffffffff; len = c->sparse ? c->sorted_entries : c->entries; for (i=0; i < len; ++i) { int huff_len = c->sparse ? lengths[values[i]] : lengths[i]; if (_vc_9894107b(c,huff_len)) { uint32 code = _vc_7f6d13b2(c->codewords[i]); int x=0, n=c->sorted_entries; while (n > 1) { int m = x + (n >> 1); if (c->sorted_codewords[m] <= code) { x = m; n -= (n>>1); } else { n >>= 1; } } assert(c->sorted_codewords[x] == code); if (c->sparse) { c->sorted_values[x] = values[i]; c->codeword_lengths[x] = huff_len; } else { c->sorted_values[x] = i; } } } }
static int _vc_d1210c1d(uint8 *data) { static uint8 vorbis[6] = { 'v', 'o', 'r', 'b', 'i', 's' }; return memcmp(data, vorbis, 6) == 0; }
static int _vc_3e2962c5(int entries, int dim) { int r = (int) floor(exp((float) log((float) entries) / dim)); if ((int) floor(pow((float) r+1, dim)) <= entries) ++r; if (pow((float) r+1, dim) <= entries) return -1; if ((int) floor(pow((float) r, dim)) > entries) return -1; return r; }
static void _vc_7d6acf31(int n, float *A, float *B, float *C) { int n4 = n >> 2, n8 = n >> 3; int k,k2; for (k=k2=0; k < n4; ++k,k2+=2) { A[k2 ] = (float) cos(4*k*M_PI/n); A[k2+1] = (float) -sin(4*k*M_PI/n); B[k2 ] = (float) cos((k2+1)*M_PI/n/2) * 0.5f; B[k2+1] = (float) sin((k2+1)*M_PI/n/2) * 0.5f; } for (k=k2=0; k < n8; ++k,k2+=2) { C[k2 ] = (float) cos(2*(k2+1)*M_PI/n); C[k2+1] = (float) -sin(2*(k2+1)*M_PI/n); } }
static void _vc_64759f10(int n, float *window) { int n2 = n >> 1, i; for (i=0; i < n2; ++i) window[i] = (float) sin(0.5 * M_PI * _vc_bfed89e8((float) sin((i - 0 + 0.5) / n2 * 0.5 * M_PI))); }
static void _vc_565398c2(int n, uint16 *rev) { int ld = _vc_98fda42c(n) - 1; int i, n8 = n >> 3; for (i=0; i < n8; ++i) rev[i] = (_vc_7f6d13b2(i) >> (32-ld+3)) << 2; }
static int _vc_b9459f6b(_Vcs *f, int b, int n) { int n2 = n >> 1, n4 = n >> 2, n8 = n >> 3; f->A[b] = (float *) setup_malloc(f, sizeof(float) * n2); f->B[b] = (float *) setup_malloc(f, sizeof(float) * n2); f->C[b] = (float *) setup_malloc(f, sizeof(float) * n4); if (!f->A[b] || !f->B[b] || !f->C[b]) return _vc_11f9578d(f, VC_OOM); _vc_7d6acf31(n, f->A[b], f->B[b], f->C[b]); f->window[b] = (float *) setup_malloc(f, sizeof(float) * n2); if (!f->window[b]) return _vc_11f9578d(f, VC_OOM); _vc_64759f10(n, f->window[b]); f->bit_reverse[b] = (uint16 *) setup_malloc(f, sizeof(uint16) * n8); if (!f->bit_reverse[b]) return _vc_11f9578d(f, VC_OOM); _vc_565398c2(n, f->bit_reverse[b]); return TRUE; }
static void _vc_a24fee51(uint16 *x, int n, int *plow, int *phigh) { int low = -1; int high = 65536; int i; for (i=0; i < n; ++i) { if (x[i] > low && x[i] < x[n]) { *plow = i; low = x[i]; } if (x[i] < high && x[i] > x[n]) { *phigh = i; high = x[i]; } } }
typedef struct { uint16 x,id; }
_VcFlOrd;
static int VC_CDECL _vc_efdc5df4(const void *p, const void *q) { _VcFlOrd *a = (_VcFlOrd *) p; _VcFlOrd *b = (_VcFlOrd *) q; return a->x < b->x ? -1 : a->x > b->x; }
#if defined(VORBISCOMPILER_NO_STDIO)
#define USE_MEMORY(z) TRUE
#else
#define USE_MEMORY(z) ((z)->stream)
#endif
static uint8 _vc_90d1fe2e(_Vcs *z) { if (USE_MEMORY(z)) { if (z->stream >= z->stream_end) { z->eof = TRUE; return 0; } return *z->stream++; }
#ifndef VORBISCOMPILER_NO_STDIO
{ int c = fgetc(z->f); if (c == EOF) { z->eof = TRUE; return 0; } return c; }
#endif
}
static uint32 _vc_a8816410(_Vcs *f) { uint32 x; x = _vc_90d1fe2e(f); x += _vc_90d1fe2e(f) << 8; x += _vc_90d1fe2e(f) << 16; x += (uint32) _vc_90d1fe2e(f) << 24; return x; }
static int _vc_fc62ee61(_Vcs *z, uint8 *data, int n) { if (USE_MEMORY(z)) { if (z->stream+n > z->stream_end) { z->eof = 1; return 0; } memcpy(data, z->stream, n); z->stream += n; return 1; }
#ifndef VORBISCOMPILER_NO_STDIO
if (fread(data, n, 1, z->f) == 1) return 1; else { z->eof = 1; return 0; }
#endif
}
static void _vc_c7e16815(_Vcs *z, int n) { if (USE_MEMORY(z)) { z->stream += n; if (z->stream >= z->stream_end) z->eof = 1; return; }
#ifndef VORBISCOMPILER_NO_STDIO
{ long x = ftell(z->f); fseek(z->f, x+n, SEEK_SET); }
#endif
}
static int _vc_d6034dc6(vc_stream *f, unsigned int loc) {
#ifndef VORBISCOMPILER_NO_PUSH
if (f->push_mode) return 0;
#endif
f->eof = 0; if (USE_MEMORY(f)) { if (f->stream_start + loc >= f->stream_end || f->stream_start + loc < f->stream_start) { f->stream = f->stream_end; f->eof = 1; return 0; } else { f->stream = f->stream_start + loc; return 1; } }
#ifndef VORBISCOMPILER_NO_STDIO
if (loc + f->f_start < loc || loc >= 0x80000000) { loc = 0x7fffffff; f->eof = 1; } else { loc += f->f_start; } if (!fseek(f->f, loc, SEEK_SET)) return 1; f->eof = 1; fseek(f->f, f->f_start, SEEK_END); return 0;
#endif
}
static uint8 _vc_ogg_hdr[4] = { 0x4f, 0x67, 0x67, 0x53 }
;
static int _vc_6cbfdccb(_Vcs *f) { if (0x4f != _vc_90d1fe2e(f)) return FALSE; if (0x67 != _vc_90d1fe2e(f)) return FALSE; if (0x67 != _vc_90d1fe2e(f)) return FALSE; if (0x53 != _vc_90d1fe2e(f)) return FALSE; return TRUE; }
#define _VC_PF_CONT 1
#define _VC_PF_FIRST 2
#define _VC_PF_LAST 4
static int _vc_04393d27(_Vcs *f) { uint32 loc0,loc1,n; if (f->first_decode && !_VC_PUSH(f)) { f->p_first.page_start = vc_get_file_offset(f) - 4; } if (0 != _vc_90d1fe2e(f)) return _vc_11f9578d(f, VC_BAD_VER); f->page_flag = _vc_90d1fe2e(f); loc0 = _vc_a8816410(f); loc1 = _vc_a8816410(f); _vc_a8816410(f); n = _vc_a8816410(f); f->last_page = n; _vc_a8816410(f); f->segment_count = _vc_90d1fe2e(f); if (!_vc_fc62ee61(f, f->segments, f->segment_count)) return _vc_11f9578d(f, VC_EOF); f->end_seg_with_known_loc = -2; if (loc0 != ~0U || loc1 != ~0U) { int i; for (i=f->segment_count-1; i >= 0; --i) if (f->segments[i] < 255) break; if (i >= 0) { f->end_seg_with_known_loc = i; f->known_loc_for_packet = loc0; } } if (f->first_decode) { int i,len; len = 0; for (i=0; i < f->segment_count; ++i) len += f->segments[i]; len += 27 + f->segment_count; f->p_first.page_end = f->p_first.page_start + len; f->p_first.last_decoded_sample = loc0; } f->next_seg = 0; return TRUE; }
static int _vc_e7208f0c(_Vcs *f) { if (!_vc_6cbfdccb(f)) return _vc_11f9578d(f, VC_NO_CAPTURE); return _vc_04393d27(f); }
static int _vc_7cf885ee(_Vcs *f) { while (f->next_seg == -1) { if (!_vc_e7208f0c(f)) return FALSE; if (f->page_flag & _VC_PF_CONT) return _vc_11f9578d(f, VC_BAD_PKT_FLAG); } f->last_seg = FALSE; f->valid_bits = 0; f->packet_bytes = 0; f->bytes_in_seg = 0; return TRUE; }
static int _vc_dace8834(_Vcs *f) { if (f->next_seg == -1) { int x = _vc_90d1fe2e(f); if (f->eof) return FALSE; if (0x4f != x ) return _vc_11f9578d(f, VC_NO_CAPTURE); if (0x67 != _vc_90d1fe2e(f)) return _vc_11f9578d(f, VC_NO_CAPTURE); if (0x67 != _vc_90d1fe2e(f)) return _vc_11f9578d(f, VC_NO_CAPTURE); if (0x53 != _vc_90d1fe2e(f)) return _vc_11f9578d(f, VC_NO_CAPTURE); if (!_vc_04393d27(f)) return FALSE; if (f->page_flag & _VC_PF_CONT) { f->last_seg = FALSE; f->bytes_in_seg = 0; return _vc_11f9578d(f, VC_BAD_PKT_FLAG); } } return _vc_7cf885ee(f); }
static int _vc_4c441ef3(_Vcs *f) { int len; if (f->last_seg) return 0; if (f->next_seg == -1) { f->last_seg_which = f->segment_count-1; if (!_vc_e7208f0c(f)) { f->last_seg = 1; return 0; } if (!(f->page_flag & _VC_PF_CONT)) return _vc_11f9578d(f, VC_BAD_PKT_FLAG); } len = f->segments[f->next_seg++]; if (len < 255) { f->last_seg = TRUE; f->last_seg_which = f->next_seg-1; } if (f->next_seg >= f->segment_count) f->next_seg = -1; assert(f->bytes_in_seg == 0); f->bytes_in_seg = len; return len; }
#define EOP (-1)
#define INVALID_BITS (-1)
static int _vc_46a0e7ca(_Vcs *f) { if (!f->bytes_in_seg) { if (f->last_seg) return EOP; else if (!_vc_4c441ef3(f)) return EOP; } assert(f->bytes_in_seg > 0); --f->bytes_in_seg; ++f->packet_bytes; return _vc_90d1fe2e(f); }
static int _vc_e113cb67(_Vcs *f) { int x = _vc_46a0e7ca(f); f->valid_bits = 0; return x; }
static int _vc_a8b1002a(_Vcs *f) { uint32 x; x = _vc_e113cb67(f); x += _vc_e113cb67(f) << 8; x += _vc_e113cb67(f) << 16; x += (uint32) _vc_e113cb67(f) << 24; return x; }
static void _vc_b2ff18b0(_Vcs *f) { while (_vc_46a0e7ca(f) != EOP); }
static uint32 _vc_a7ba7315(_Vcs *f, int n) { uint32 z; if (f->valid_bits < 0) return 0; if (f->valid_bits < n) { if (n > 24) { z = _vc_a7ba7315(f, 24); z += _vc_a7ba7315(f, n-24) << 24; return z; } if (f->valid_bits == 0) f->acc = 0; while (f->valid_bits < n) { int z = _vc_46a0e7ca(f); if (z == EOP) { f->valid_bits = INVALID_BITS; return 0; } f->acc += z << f->valid_bits; f->valid_bits += 8; } } assert(f->valid_bits >= n); z = f->acc & ((1 << n)-1); f->acc >>= n; f->valid_bits -= n; return z; }
static __forceinline void _vc_91773c6e(_Vcs *f) { if (f->valid_bits <= 24) { if (f->valid_bits == 0) f->acc = 0; do { int z; if (f->last_seg && !f->bytes_in_seg) return; z = _vc_46a0e7ca(f); if (z == EOP) return; f->acc += (unsigned) z << f->valid_bits; f->valid_bits += 8; } while (f->valid_bits <= 24); } }
enum { VC_PKT_ID = 1, VC_PKT_COMMENT = 3, VC_PKT_SETUP = 5 }
;
static int _vc_31ce9124(_Vcs *f, _VcCb *c) { int i; _vc_91773c6e(f); if (c->codewords == NULL && c->sorted_codewords == NULL) return -1; if (c->entries > 8 ? c->sorted_codewords!=NULL : !c->codewords) { uint32 code = _vc_7f6d13b2(f->acc); int x=0, n=c->sorted_entries, len; while (n > 1) { int m = x + (n >> 1); if (c->sorted_codewords[m] <= code) { x = m; n -= (n>>1); } else { n >>= 1; } } if (!c->sparse) x = c->sorted_values[x]; len = c->codeword_lengths[x]; if (f->valid_bits >= len) { f->acc >>= len; f->valid_bits -= len; return x; } f->valid_bits = 0; return -1; } assert(!c->sparse); for (i=0; i < c->entries; ++i) { if (c->codeword_lengths[i] == _VC_NC) continue; if (c->codewords[i] == (f->acc & ((1 << c->codeword_lengths[i])-1))) { if (f->valid_bits >= c->codeword_lengths[i]) { f->acc >>= c->codeword_lengths[i]; f->valid_bits -= c->codeword_lengths[i]; return i; } f->valid_bits = 0; return -1; } } _vc_11f9578d(f, VC_BAD_STREAM); f->valid_bits = 0; return -1; }
#ifndef VORBISCOMPILER_NO_INLINE_DEC
#define DECODE_RAW(var, f,c) \
if (f->valid_bits < VORBISCOMPILER_HUFF_BITS) \
_vc_91773c6e(f); \
var = f->acc & _VC_HUFF_MSK; \
var = c->fast_huffman[var]; \
if (var >= 0) { \
int n = c->codeword_lengths[var]; \
f->acc >>= n; \
f->valid_bits -= n; \
if (f->valid_bits < 0) { f->valid_bits = 0; var = -1; } \
} else { \
var = _vc_31ce9124(f,c); \
}
#else
static int _vc_d3637903(_Vcs *f, _VcCb *c) { int i; if (f->valid_bits < VORBISCOMPILER_HUFF_BITS) _vc_91773c6e(f); i = f->acc & _VC_HUFF_MSK; i = c->fast_huffman[i]; if (i >= 0) { f->acc >>= c->codeword_lengths[i]; f->valid_bits -= c->codeword_lengths[i]; if (f->valid_bits < 0) { f->valid_bits = 0; return -1; } return i; } return _vc_31ce9124(f,c); }
#define DECODE_RAW(var,f,c) var = _vc_d3637903(f,c);
#endif
#define DECODE(var,f,c) \
DECODE_RAW(var,f,c) \
if (c->sparse) var = c->sorted_values[var];
#ifndef VORBISCOMPILER_DIV_CODEBOOK
#define DECODE_VQ(var,f,c) DECODE_RAW(var,f,c)
#else
#define DECODE_VQ(var,f,c) DECODE(var,f,c)
#endif
#define CODEBOOK_ELEMENT(c,off) (c->multiplicands[off])
#define CODEBOOK_ELEMENT_FAST(c,off) (c->multiplicands[off])
#define CODEBOOK_ELEMENT_BASE(c) (0)
static int _vc_57a58317(_Vcs *f, _VcCb *c) { int z = -1; if (c->lookup_type == 0) _vc_11f9578d(f, VC_BAD_STREAM); else { DECODE_VQ(z,f,c); if (c->sparse) assert(z < c->sorted_entries); if (z < 0) { if (!f->bytes_in_seg) if (f->last_seg) return z; _vc_11f9578d(f, VC_BAD_STREAM); } } return z; }
static int _vc_bdb9765b(_Vcs *f, _VcCb *c, float *output, int len) { int i,z = _vc_57a58317(f,c); if (z < 0) return FALSE; if (len > c->dimensions) len = c->dimensions;
#ifdef VORBISCOMPILER_DIV_CODEBOOK
if (c->lookup_type == 1) { float last = CODEBOOK_ELEMENT_BASE(c); int div = 1; for (i=0; i < len; ++i) { int off = (z / div) % c->lookup_values; float val = CODEBOOK_ELEMENT_FAST(c,off) + last; output[i] += val; if (c->sequence_p) last = val + c->minimum_value; div *= c->lookup_values; } return TRUE; }
#endif
z *= c->dimensions; if (c->sequence_p) { float last = CODEBOOK_ELEMENT_BASE(c); for (i=0; i < len; ++i) { float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last; output[i] += val; last = val + c->minimum_value; } } else { float last = CODEBOOK_ELEMENT_BASE(c); for (i=0; i < len; ++i) { output[i] += CODEBOOK_ELEMENT_FAST(c,z+i) + last; } } return TRUE; }
static int _vc_3b72c818(_Vcs *f, _VcCb *c, float *output, int len, int step) { int i,z = _vc_57a58317(f,c); float last = CODEBOOK_ELEMENT_BASE(c); if (z < 0) return FALSE; if (len > c->dimensions) len = c->dimensions;
#ifdef VORBISCOMPILER_DIV_CODEBOOK
if (c->lookup_type == 1) { int div = 1; for (i=0; i < len; ++i) { int off = (z / div) % c->lookup_values; float val = CODEBOOK_ELEMENT_FAST(c,off) + last; output[i*step] += val; if (c->sequence_p) last = val; div *= c->lookup_values; } return TRUE; }
#endif
z *= c->dimensions; for (i=0; i < len; ++i) { float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last; output[i*step] += val; if (c->sequence_p) last = val; } return TRUE; }
static int _vc_1bf1906b(_Vcs *f, _VcCb *c, float **outputs, int ch, int *c_inter_p, int *p_inter_p, int len, int total_decode) { int c_inter = *c_inter_p; int p_inter = *p_inter_p; int i,z, effective = c->dimensions; if (c->lookup_type == 0) return _vc_11f9578d(f, VC_BAD_STREAM); while (total_decode > 0) { float last = CODEBOOK_ELEMENT_BASE(c); DECODE_VQ(z,f,c);
#ifndef VORBISCOMPILER_DIV_CODEBOOK
assert(!c->sparse || z < c->sorted_entries);
#endif
if (z < 0) { if (!f->bytes_in_seg) if (f->last_seg) return FALSE; return _vc_11f9578d(f, VC_BAD_STREAM); } if (c_inter + p_inter*ch + effective > len * ch) { effective = len*ch - (p_inter*ch - c_inter); }
#ifdef VORBISCOMPILER_DIV_CODEBOOK
if (c->lookup_type == 1) { int div = 1; for (i=0; i < effective; ++i) { int off = (z / div) % c->lookup_values; float val = CODEBOOK_ELEMENT_FAST(c,off) + last; if (outputs[c_inter]) outputs[c_inter][p_inter] += val; if (++c_inter == ch) { c_inter = 0; ++p_inter; } if (c->sequence_p) last = val; div *= c->lookup_values; } } else
#endif
{ z *= c->dimensions; if (c->sequence_p) { for (i=0; i < effective; ++i) { float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last; if (outputs[c_inter]) outputs[c_inter][p_inter] += val; if (++c_inter == ch) { c_inter = 0; ++p_inter; } last = val; } } else { for (i=0; i < effective; ++i) { float val = CODEBOOK_ELEMENT_FAST(c,z+i) + last; if (outputs[c_inter]) outputs[c_inter][p_inter] += val; if (++c_inter == ch) { c_inter = 0; ++p_inter; } } } } total_decode -= effective; } *c_inter_p = c_inter; *p_inter_p = p_inter; return TRUE; }
static int _vc_3f512384(int x, int x0, int x1, int y0, int y1) { int dy = y1 - y0; int adx = x1 - x0; int err = abs(dy) * (x - x0); int off = err / adx; return dy < 0 ? y0 - off : y0 + off; }
static float inverse_db_table[256] = { 1.0649863e-07f, 1.1341951e-07f, 1.2079015e-07f, 1.2863978e-07f, 1.3699951e-07f, 1.4590251e-07f, 1.5538408e-07f, 1.6548181e-07f, 1.7623575e-07f, 1.8768855e-07f, 1.9988561e-07f, 2.1287530e-07f, 2.2670913e-07f, 2.4144197e-07f, 2.5713223e-07f, 2.7384213e-07f, 2.9163793e-07f, 3.1059021e-07f, 3.3077411e-07f, 3.5226968e-07f, 3.7516214e-07f, 3.9954229e-07f, 4.2550680e-07f, 4.5315863e-07f, 4.8260743e-07f, 5.1396998e-07f, 5.4737065e-07f, 5.8294187e-07f, 6.2082472e-07f, 6.6116941e-07f, 7.0413592e-07f, 7.4989464e-07f, 7.9862701e-07f, 8.5052630e-07f, 9.0579828e-07f, 9.6466216e-07f, 1.0273513e-06f, 1.0941144e-06f, 1.1652161e-06f, 1.2409384e-06f, 1.3215816e-06f, 1.4074654e-06f, 1.4989305e-06f, 1.5963394e-06f, 1.7000785e-06f, 1.8105592e-06f, 1.9282195e-06f, 2.0535261e-06f, 2.1869758e-06f, 2.3290978e-06f, 2.4804557e-06f, 2.6416497e-06f, 2.8133190e-06f, 2.9961443e-06f, 3.1908506e-06f, 3.3982101e-06f, 3.6190449e-06f, 3.8542308e-06f, 4.1047004e-06f, 4.3714470e-06f, 4.6555282e-06f, 4.9580707e-06f, 5.2802740e-06f, 5.6234160e-06f, 5.9888572e-06f, 6.3780469e-06f, 6.7925283e-06f, 7.2339451e-06f, 7.7040476e-06f, 8.2047000e-06f, 8.7378876e-06f, 9.3057248e-06f, 9.9104632e-06f, 1.0554501e-05f, 1.1240392e-05f, 1.1970856e-05f, 1.2748789e-05f, 1.3577278e-05f, 1.4459606e-05f, 1.5399272e-05f, 1.6400004e-05f, 1.7465768e-05f, 1.8600792e-05f, 1.9809576e-05f, 2.1096914e-05f, 2.2467911e-05f, 2.3928002e-05f, 2.5482978e-05f, 2.7139006e-05f, 2.8902651e-05f, 3.0780908e-05f, 3.2781225e-05f, 3.4911534e-05f, 3.7180282e-05f, 3.9596466e-05f, 4.2169667e-05f, 4.4910090e-05f, 4.7828601e-05f, 5.0936773e-05f, 5.4246931e-05f, 5.7772202e-05f, 6.1526565e-05f, 6.5524908e-05f, 6.9783085e-05f, 7.4317983e-05f, 7.9147585e-05f, 8.4291040e-05f, 8.9768747e-05f, 9.5602426e-05f, 0.00010181521f, 0.00010843174f, 0.00011547824f, 0.00012298267f, 0.00013097477f, 0.00013948625f, 0.00014855085f, 0.00015820453f, 0.00016848555f, 0.00017943469f, 0.00019109536f, 0.00020351382f, 0.00021673929f, 0.00023082423f, 0.00024582449f, 0.00026179955f, 0.00027881276f, 0.00029693158f, 0.00031622787f, 0.00033677814f, 0.00035866388f, 0.00038197188f, 0.00040679456f, 0.00043323036f, 0.00046138411f, 0.00049136745f, 0.00052329927f, 0.00055730621f, 0.00059352311f, 0.00063209358f, 0.00067317058f, 0.00071691700f, 0.00076350630f, 0.00081312324f, 0.00086596457f, 0.00092223983f, 0.00098217216f, 0.0010459992f, 0.0011139742f, 0.0011863665f, 0.0012634633f, 0.0013455702f, 0.0014330129f, 0.0015261382f, 0.0016253153f, 0.0017309374f, 0.0018434235f, 0.0019632195f, 0.0020908006f, 0.0022266726f, 0.0023713743f, 0.0025254795f, 0.0026895994f, 0.0028643847f, 0.0030505286f, 0.0032487691f, 0.0034598925f, 0.0036847358f, 0.0039241906f, 0.0041792066f, 0.0044507950f, 0.0047400328f, 0.0050480668f, 0.0053761186f, 0.0057254891f, 0.0060975636f, 0.0064938176f, 0.0069158225f, 0.0073652516f, 0.0078438871f, 0.0083536271f, 0.0088964928f, 0.009474637f, 0.010090352f, 0.010746080f, 0.011444421f, 0.012188144f, 0.012980198f, 0.013823725f, 0.014722068f, 0.015678791f, 0.016697687f, 0.017782797f, 0.018938423f, 0.020169149f, 0.021479854f, 0.022875735f, 0.024362330f, 0.025945531f, 0.027631618f, 0.029427276f, 0.031339626f, 0.033376252f, 0.035545228f, 0.037855157f, 0.040315199f, 0.042935108f, 0.045725273f, 0.048696758f, 0.051861348f, 0.055231591f, 0.058820850f, 0.062643361f, 0.066714279f, 0.071049749f, 0.075666962f, 0.080584227f, 0.085821044f, 0.091398179f, 0.097337747f, 0.10366330f, 0.11039993f, 0.11757434f, 0.12521498f, 0.13335215f, 0.14201813f, 0.15124727f, 0.16107617f, 0.17154380f, 0.18269168f, 0.19456402f, 0.20720788f, 0.22067342f, 0.23501402f, 0.25028656f, 0.26655159f, 0.28387361f, 0.30232132f, 0.32196786f, 0.34289114f, 0.36517414f, 0.38890521f, 0.41417847f, 0.44109412f, 0.46975890f, 0.50028648f, 0.53279791f, 0.56742212f, 0.60429640f, 0.64356699f, 0.68538959f, 0.72993007f, 0.77736504f, 0.82788260f, 0.88168307f, 0.9389798f, 1.0f }
;
#ifndef VORBISCOMPILER_NO_DEFER_FLR
#define LINE_OP(a,b) a *= b
#else
#define LINE_OP(a,b) a = b
#endif
#ifdef VORBISCOMPILER_DIV_TBL
#define DIVTAB_NUMER 32
#define DIVTAB_DENOM 64
int8 integer_divide_table[DIVTAB_NUMER][DIVTAB_DENOM];
#endif
static __forceinline void _vc_da904faf(float *output, int x0, int y0, int x1, int y1, int n) { int dy = y1 - y0; int adx = x1 - x0; int ady = abs(dy); int base; int x=x0,y=y0; int err = 0; int sy;
#ifdef VORBISCOMPILER_DIV_TBL
if (adx < DIVTAB_DENOM && ady < DIVTAB_NUMER) { if (dy < 0) { base = -integer_divide_table[ady][adx]; sy = base-1; } else { base = integer_divide_table[ady][adx]; sy = base+1; } } else { base = dy / adx; if (dy < 0) sy = base - 1; else sy = base+1; }
#else
base = dy / adx; if (dy < 0) sy = base - 1; else sy = base+1;
#endif
ady -= abs(base) * adx; if (x1 > n) x1 = n; if (x < x1) { LINE_OP(output[x], inverse_db_table[y&255]); for (++x; x < x1; ++x) { err += ady; if (err >= adx) { err -= adx; y += sy; } else y += base; LINE_OP(output[x], inverse_db_table[y&255]); } } }
static int _vc_2a399e91(_Vcs *f, _VcCb *book, float *target, int offset, int n, int rtype) { int k; if (rtype == 0) { int step = n / book->dimensions; for (k=0; k < step; ++k) if (!_vc_3b72c818(f, book, target+offset+k, n-offset-k, step)) return FALSE; } else { for (k=0; k < n; ) { if (!_vc_bdb9765b(f, book, target+offset, n-k)) return FALSE; k += book->dimensions; offset += book->dimensions; } } return TRUE; }
static void _vc_a67261bf(_Vcs *f, float *residue_buffers[], int ch, int n, int rn, uint8 *do_not_decode) { int i,j,pass; _VcRs *r = f->residue_config + rn; int rtype = f->residue_types[rn]; int c = r->classbook; int classwords = f->codebooks[c].dimensions; unsigned int actual_size = rtype == 2 ? n*2 : n; unsigned int limit_r_begin = (r->begin < actual_size ? r->begin : actual_size); unsigned int limit_r_end = (r->end < actual_size ? r->end : actual_size); int n_read = limit_r_end - limit_r_begin; int part_read = n_read / r->part_size; int temp_alloc_point = temp_alloc_save(f);
#ifndef VORBISCOMPILER_DIV_RESIDUE
uint8 ***part_classdata = (uint8 ***) temp_block_array(f,f->channels, part_read * sizeof(**part_classdata));
#else
int **classifications = (int **) temp_block_array(f,f->channels, part_read * sizeof(**classifications));
#endif
CHECK(f); for (i=0; i < ch; ++i) if (!do_not_decode[i]) memset(residue_buffers[i], 0, sizeof(float) * n); if (rtype == 2 && ch != 1) { for (j=0; j < ch; ++j) if (!do_not_decode[j]) break; if (j == ch) goto done; for (pass=0; pass < 8; ++pass) { int pcount = 0, class_set = 0; if (ch == 2) { while (pcount < part_read) { int z = r->begin + pcount*r->part_size; int c_inter = (z & 1), p_inter = z>>1; if (pass == 0) { _VcCb *c = f->codebooks+r->classbook; int q; DECODE(q,f,c); if (q == EOP) goto done;
#ifndef VORBISCOMPILER_DIV_RESIDUE
part_classdata[0][class_set] = r->classdata[q];
#else
for (i=classwords-1; i >= 0; --i) { classifications[0][i+pcount] = q % r->classifications; q /= r->classifications; }
#endif
} for (i=0; i < classwords && pcount < part_read; ++i, ++pcount) { int z = r->begin + pcount*r->part_size;
#ifndef VORBISCOMPILER_DIV_RESIDUE
int c = part_classdata[0][class_set][i];
#else
int c = classifications[0][pcount];
#endif
int b = r->residue_books[c][pass]; if (b >= 0) { _VcCb *book = f->codebooks + b;
#ifdef VORBISCOMPILER_DIV_CODEBOOK
if (!_vc_1bf1906b(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size)) goto done;
#else
if (!_vc_1bf1906b(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size)) goto done;
#endif
} else { z += r->part_size; c_inter = z & 1; p_inter = z >> 1; } }
#ifndef VORBISCOMPILER_DIV_RESIDUE
++class_set;
#endif
} } else if (ch > 2) { while (pcount < part_read) { int z = r->begin + pcount*r->part_size; int c_inter = z % ch, p_inter = z/ch; if (pass == 0) { _VcCb *c = f->codebooks+r->classbook; int q; DECODE(q,f,c); if (q == EOP) goto done;
#ifndef VORBISCOMPILER_DIV_RESIDUE
part_classdata[0][class_set] = r->classdata[q];
#else
for (i=classwords-1; i >= 0; --i) { classifications[0][i+pcount] = q % r->classifications; q /= r->classifications; }
#endif
} for (i=0; i < classwords && pcount < part_read; ++i, ++pcount) { int z = r->begin + pcount*r->part_size;
#ifndef VORBISCOMPILER_DIV_RESIDUE
int c = part_classdata[0][class_set][i];
#else
int c = classifications[0][pcount];
#endif
int b = r->residue_books[c][pass]; if (b >= 0) { _VcCb *book = f->codebooks + b; if (!_vc_1bf1906b(f, book, residue_buffers, ch, &c_inter, &p_inter, n, r->part_size)) goto done; } else { z += r->part_size; c_inter = z % ch; p_inter = z / ch; } }
#ifndef VORBISCOMPILER_DIV_RESIDUE
++class_set;
#endif
} } } goto done; } CHECK(f); for (pass=0; pass < 8; ++pass) { int pcount = 0, class_set=0; while (pcount < part_read) { if (pass == 0) { for (j=0; j < ch; ++j) { if (!do_not_decode[j]) { _VcCb *c = f->codebooks+r->classbook; int temp; DECODE(temp,f,c); if (temp == EOP) goto done;
#ifndef VORBISCOMPILER_DIV_RESIDUE
part_classdata[j][class_set] = r->classdata[temp];
#else
for (i=classwords-1; i >= 0; --i) { classifications[j][i+pcount] = temp % r->classifications; temp /= r->classifications; }
#endif
} } } for (i=0; i < classwords && pcount < part_read; ++i, ++pcount) { for (j=0; j < ch; ++j) { if (!do_not_decode[j]) {
#ifndef VORBISCOMPILER_DIV_RESIDUE
int c = part_classdata[j][class_set][i];
#else
int c = classifications[j][pcount];
#endif
int b = r->residue_books[c][pass]; if (b >= 0) { float *target = residue_buffers[j]; int offset = r->begin + pcount * r->part_size; int n = r->part_size; _VcCb *book = f->codebooks + b; if (!_vc_2a399e91(f, book, target, offset, n, rtype)) goto done; } } } }
#ifndef VORBISCOMPILER_DIV_RESIDUE
++class_set;
#endif
} } done: CHECK(f);
#ifndef VORBISCOMPILER_DIV_RESIDUE
temp_free(f,part_classdata);
#else
temp_free(f,classifications);
#endif
temp_alloc_restore(f,temp_alloc_point); }
#if 0
void inverse_mdct_slow(float *buffer, int n) { int i,j; int n2 = n >> 1; float *x = (float *) malloc(sizeof(*x) * n2); memcpy(x, buffer, sizeof(*x) * n2); for (i=0; i < n; ++i) { float acc = 0; for (j=0; j < n2; ++j) acc += x[j] * (float) cos(M_PI / 2 / n * (2 * i + 1 + n/2.0)*(2*j+1)); buffer[i] = acc; } free(x); }
#elif 0
void inverse_mdct_slow(float *buffer, int n, _Vcs *f, int blocktype) { float mcos[16384]; int i,j; int n2 = n >> 1, nmask = (n << 2) -1; float *x = (float *) malloc(sizeof(*x) * n2); memcpy(x, buffer, sizeof(*x) * n2); for (i=0; i < 4*n; ++i) mcos[i] = (float) cos(M_PI / 2 * i / n); for (i=0; i < n; ++i) { float acc = 0; for (j=0; j < n2; ++j) acc += x[j] * mcos[(2 * i + 1 + n2)*(2*j+1) & nmask]; buffer[i] = acc; } free(x); }
#elif 0
void dct_iv_slow(float *buffer, int n) { float mcos[16384]; float x[2048]; int i,j; int n2 = n >> 1, nmask = (n << 3) - 1; memcpy(x, buffer, sizeof(*x) * n); for (i=0; i < 8*n; ++i) mcos[i] = (float) cos(M_PI / 4 * i / n); for (i=0; i < n; ++i) { float acc = 0; for (j=0; j < n; ++j) acc += x[j] * mcos[((2 * i + 1)*(2*j+1)) & nmask]; buffer[i] = acc; } }
void inverse_mdct_slow(float *buffer, int n, _Vcs *f, int blocktype) { int i, n4 = n >> 2, n2 = n >> 1, n3_4 = n - n4; float temp[4096]; memcpy(temp, buffer, n2 * sizeof(float)); dct_iv_slow(temp, n2); for (i=0; i < n4 ; ++i) buffer[i] = temp[i+n4]; for ( ; i < n3_4; ++i) buffer[i] = -temp[n3_4 - i - 1]; for ( ; i < n ; ++i) buffer[i] = -temp[i - n3_4]; }
#endif
#ifndef _VC_LIB_MDCT
#define _VC_LIB_MDCT 0
#endif
#if _VC_LIB_MDCT
typedef struct { int n; int log2n; float *trig; int *bitrev; float scale; }
mdct_lookup;
extern void mdct_init(mdct_lookup *lookup, int n);
extern void mdct_clear(mdct_lookup *l);
extern void mdct_backward(mdct_lookup *init, float *in, float *out);
mdct_lookup M1,M2;
void _vc_87664557(float *buffer, int n, _Vcs *f, int blocktype) { mdct_lookup *M; if (M1.n == n) M = &M1; else if (M2.n == n) M = &M2; else if (M1.n == 0) { mdct_init(&M1, n); M = &M1; } else { if (M2.n) __asm int 3; mdct_init(&M2, n); M = &M2; } mdct_backward(M, buffer, buffer); }
#endif
static void _vc_f04f829c(int n, float *e, int i_off, int k_off, float *A) { float *ee0 = e + i_off; float *ee2 = ee0 + k_off; int i; assert((n & 3) == 0); for (i=(n>>2); i > 0; --i) { float k00_20, k01_21; k00_20 = ee0[ 0] - ee2[ 0]; k01_21 = ee0[-1] - ee2[-1]; ee0[ 0] += ee2[ 0]; ee0[-1] += ee2[-1]; ee2[ 0] = k00_20 * A[0] - k01_21 * A[1]; ee2[-1] = k01_21 * A[0] + k00_20 * A[1]; A += 8; k00_20 = ee0[-2] - ee2[-2]; k01_21 = ee0[-3] - ee2[-3]; ee0[-2] += ee2[-2]; ee0[-3] += ee2[-3]; ee2[-2] = k00_20 * A[0] - k01_21 * A[1]; ee2[-3] = k01_21 * A[0] + k00_20 * A[1]; A += 8; k00_20 = ee0[-4] - ee2[-4]; k01_21 = ee0[-5] - ee2[-5]; ee0[-4] += ee2[-4]; ee0[-5] += ee2[-5]; ee2[-4] = k00_20 * A[0] - k01_21 * A[1]; ee2[-5] = k01_21 * A[0] + k00_20 * A[1]; A += 8; k00_20 = ee0[-6] - ee2[-6]; k01_21 = ee0[-7] - ee2[-7]; ee0[-6] += ee2[-6]; ee0[-7] += ee2[-7]; ee2[-6] = k00_20 * A[0] - k01_21 * A[1]; ee2[-7] = k01_21 * A[0] + k00_20 * A[1]; A += 8; ee0 -= 8; ee2 -= 8; } }
static void _vc_c4b03c2a(int lim, float *e, int d0, int k_off, float *A, int k1) { int i; float k00_20, k01_21; float *e0 = e + d0; float *e2 = e0 + k_off; for (i=lim >> 2; i > 0; --i) { k00_20 = e0[-0] - e2[-0]; k01_21 = e0[-1] - e2[-1]; e0[-0] += e2[-0]; e0[-1] += e2[-1]; e2[-0] = (k00_20)*A[0] - (k01_21) * A[1]; e2[-1] = (k01_21)*A[0] + (k00_20) * A[1]; A += k1; k00_20 = e0[-2] - e2[-2]; k01_21 = e0[-3] - e2[-3]; e0[-2] += e2[-2]; e0[-3] += e2[-3]; e2[-2] = (k00_20)*A[0] - (k01_21) * A[1]; e2[-3] = (k01_21)*A[0] + (k00_20) * A[1]; A += k1; k00_20 = e0[-4] - e2[-4]; k01_21 = e0[-5] - e2[-5]; e0[-4] += e2[-4]; e0[-5] += e2[-5]; e2[-4] = (k00_20)*A[0] - (k01_21) * A[1]; e2[-5] = (k01_21)*A[0] + (k00_20) * A[1]; A += k1; k00_20 = e0[-6] - e2[-6]; k01_21 = e0[-7] - e2[-7]; e0[-6] += e2[-6]; e0[-7] += e2[-7]; e2[-6] = (k00_20)*A[0] - (k01_21) * A[1]; e2[-7] = (k01_21)*A[0] + (k00_20) * A[1]; e0 -= 8; e2 -= 8; A += k1; } }
static void _vc_c6df9477(int n, float *e, int i_off, int k_off, float *A, int a_off, int k0) { int i; float A0 = A[0]; float A1 = A[0+1]; float A2 = A[0+a_off]; float A3 = A[0+a_off+1]; float A4 = A[0+a_off*2+0]; float A5 = A[0+a_off*2+1]; float A6 = A[0+a_off*3+0]; float A7 = A[0+a_off*3+1]; float k00,k11; float *ee0 = e +i_off; float *ee2 = ee0+k_off; for (i=n; i > 0; --i) { k00 = ee0[ 0] - ee2[ 0]; k11 = ee0[-1] - ee2[-1]; ee0[ 0] = ee0[ 0] + ee2[ 0]; ee0[-1] = ee0[-1] + ee2[-1]; ee2[ 0] = (k00) * A0 - (k11) * A1; ee2[-1] = (k11) * A0 + (k00) * A1; k00 = ee0[-2] - ee2[-2]; k11 = ee0[-3] - ee2[-3]; ee0[-2] = ee0[-2] + ee2[-2]; ee0[-3] = ee0[-3] + ee2[-3]; ee2[-2] = (k00) * A2 - (k11) * A3; ee2[-3] = (k11) * A2 + (k00) * A3; k00 = ee0[-4] - ee2[-4]; k11 = ee0[-5] - ee2[-5]; ee0[-4] = ee0[-4] + ee2[-4]; ee0[-5] = ee0[-5] + ee2[-5]; ee2[-4] = (k00) * A4 - (k11) * A5; ee2[-5] = (k11) * A4 + (k00) * A5; k00 = ee0[-6] - ee2[-6]; k11 = ee0[-7] - ee2[-7]; ee0[-6] = ee0[-6] + ee2[-6]; ee0[-7] = ee0[-7] + ee2[-7]; ee2[-6] = (k00) * A6 - (k11) * A7; ee2[-7] = (k11) * A6 + (k00) * A7; ee0 -= k0; ee2 -= k0; } }
static __forceinline void _vc_2ebcb333(float *z) { float k00,k11,k22,k33; float y0,y1,y2,y3; k00 = z[ 0] - z[-4]; y0 = z[ 0] + z[-4]; y2 = z[-2] + z[-6]; k22 = z[-2] - z[-6]; z[-0] = y0 + y2; z[-2] = y0 - y2; k33 = z[-3] - z[-7]; z[-4] = k00 + k33; z[-6] = k00 - k33; k11 = z[-1] - z[-5]; y1 = z[-1] + z[-5]; y3 = z[-3] + z[-7]; z[-1] = y1 + y3; z[-3] = y1 - y3; z[-5] = k11 - k22; z[-7] = k11 + k22; }
static void _vc_bb3ddf2e(int n, float *e, int i_off, float *A, int base_n) { int a_off = base_n >> 3; float A2 = A[0+a_off]; float *z = e + i_off; float *base = z - 16 * n; while (z > base) { float k00,k11; float l00,l11; k00 = z[-0] - z[ -8]; k11 = z[-1] - z[ -9]; l00 = z[-2] - z[-10]; l11 = z[-3] - z[-11]; z[ -0] = z[-0] + z[ -8]; z[ -1] = z[-1] + z[ -9]; z[ -2] = z[-2] + z[-10]; z[ -3] = z[-3] + z[-11]; z[ -8] = k00; z[ -9] = k11; z[-10] = (l00+l11) * A2; z[-11] = (l11-l00) * A2; k00 = z[ -4] - z[-12]; k11 = z[ -5] - z[-13]; l00 = z[ -6] - z[-14]; l11 = z[ -7] - z[-15]; z[ -4] = z[ -4] + z[-12]; z[ -5] = z[ -5] + z[-13]; z[ -6] = z[ -6] + z[-14]; z[ -7] = z[ -7] + z[-15]; z[-12] = k11; z[-13] = -k00; z[-14] = (l11-l00) * A2; z[-15] = (l00+l11) * -A2; _vc_2ebcb333(z); _vc_2ebcb333(z-8); z -= 16; } }
static void _vc_87664557(float *buffer, int n, _Vcs *f, int blocktype) { int n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, l; int ld; int save_point = temp_alloc_save(f); float *buf2 = (float *) temp_alloc(f, n2 * sizeof(*buf2)); float *u=NULL,*v=NULL; float *A = f->A[blocktype]; { float *d,*e, *AA, *e_stop; d = &buf2[n2-2]; AA = A; e = &buffer[0]; e_stop = &buffer[n2]; while (e != e_stop) { d[1] = (e[0] * AA[0] - e[2]*AA[1]); d[0] = (e[0] * AA[1] + e[2]*AA[0]); d -= 2; AA += 2; e += 4; } e = &buffer[n2-3]; while (d >= buf2) { d[1] = (-e[2] * AA[0] - -e[0]*AA[1]); d[0] = (-e[2] * AA[1] + -e[0]*AA[0]); d -= 2; AA += 2; e -= 4; } } u = buffer; v = buf2; { float *AA = &A[n2-8]; float *d0,*d1, *e0, *e1; e0 = &v[n4]; e1 = &v[0]; d0 = &u[n4]; d1 = &u[0]; while (AA >= A) { float v40_20, v41_21; v41_21 = e0[1] - e1[1]; v40_20 = e0[0] - e1[0]; d0[1] = e0[1] + e1[1]; d0[0] = e0[0] + e1[0]; d1[1] = v41_21*AA[4] - v40_20*AA[5]; d1[0] = v40_20*AA[4] + v41_21*AA[5]; v41_21 = e0[3] - e1[3]; v40_20 = e0[2] - e1[2]; d0[3] = e0[3] + e1[3]; d0[2] = e0[2] + e1[2]; d1[3] = v41_21*AA[0] - v40_20*AA[1]; d1[2] = v40_20*AA[0] + v41_21*AA[1]; AA -= 8; d0 += 4; d1 += 4; e0 += 4; e1 += 4; } } ld = _vc_98fda42c(n) - 1; _vc_f04f829c(n >> 4, u, n2-1-n4*0, -(n >> 3), A); _vc_f04f829c(n >> 4, u, n2-1-n4*1, -(n >> 3), A); _vc_c4b03c2a(n >> 5, u, n2-1 - n8*0, -(n >> 4), A, 16); _vc_c4b03c2a(n >> 5, u, n2-1 - n8*1, -(n >> 4), A, 16); _vc_c4b03c2a(n >> 5, u, n2-1 - n8*2, -(n >> 4), A, 16); _vc_c4b03c2a(n >> 5, u, n2-1 - n8*3, -(n >> 4), A, 16); l=2; for (; l < (ld-3)>>1; ++l) { int k0 = n >> (l+2), k0_2 = k0>>1; int lim = 1 << (l+1); int i; for (i=0; i < lim; ++i) _vc_c4b03c2a(n >> (l+4), u, n2-1 - k0*i, -k0_2, A, 1 << (l+3)); } for (; l < ld-6; ++l) { int k0 = n >> (l+2), k1 = 1 << (l+3), k0_2 = k0>>1; int rlim = n >> (l+6), r; int lim = 1 << (l+1); int i_off; float *A0 = A; i_off = n2-1; for (r=rlim; r > 0; --r) { _vc_c6df9477(lim, u, i_off, -k0_2, A0, k1, k0); A0 += k1*4; i_off -= 8; } } _vc_bb3ddf2e(n >> 5, u, n2-1, A, n); { uint16 *bitrev = f->bit_reverse[blocktype]; float *d0 = &v[n4-4]; float *d1 = &v[n2-4]; while (d0 >= v) { int k4; k4 = bitrev[0]; d1[3] = u[k4+0]; d1[2] = u[k4+1]; d0[3] = u[k4+2]; d0[2] = u[k4+3]; k4 = bitrev[1]; d1[1] = u[k4+0]; d1[0] = u[k4+1]; d0[1] = u[k4+2]; d0[0] = u[k4+3]; d0 -= 4; d1 -= 4; bitrev += 2; } } assert(v == buf2); { float *C = f->C[blocktype]; float *d, *e; d = v; e = v + n2 - 4; while (d < e) { float a02,a11,b0,b1,b2,b3; a02 = d[0] - e[2]; a11 = d[1] + e[3]; b0 = C[1]*a02 + C[0]*a11; b1 = C[1]*a11 - C[0]*a02; b2 = d[0] + e[ 2]; b3 = d[1] - e[ 3]; d[0] = b2 + b0; d[1] = b3 + b1; e[2] = b2 - b0; e[3] = b1 - b3; a02 = d[2] - e[0]; a11 = d[3] + e[1]; b0 = C[3]*a02 + C[2]*a11; b1 = C[3]*a11 - C[2]*a02; b2 = d[2] + e[ 0]; b3 = d[3] - e[ 1]; d[2] = b2 + b0; d[3] = b3 + b1; e[0] = b2 - b0; e[1] = b1 - b3; C += 4; d += 4; e -= 4; } } { float *d0,*d1,*d2,*d3; float *B = f->B[blocktype] + n2 - 8; float *e = buf2 + n2 - 8; d0 = &buffer[0]; d1 = &buffer[n2-4]; d2 = &buffer[n2]; d3 = &buffer[n-4]; while (e >= v) { float p0,p1,p2,p3; p3 = e[6]*B[7] - e[7]*B[6]; p2 = -e[6]*B[6] - e[7]*B[7]; d0[0] = p3; d1[3] = - p3; d2[0] = p2; d3[3] = p2; p1 = e[4]*B[5] - e[5]*B[4]; p0 = -e[4]*B[4] - e[5]*B[5]; d0[1] = p1; d1[2] = - p1; d2[1] = p0; d3[2] = p0; p3 = e[2]*B[3] - e[3]*B[2]; p2 = -e[2]*B[2] - e[3]*B[3]; d0[2] = p3; d1[1] = - p3; d2[2] = p2; d3[1] = p2; p1 = e[0]*B[1] - e[1]*B[0]; p0 = -e[0]*B[0] - e[1]*B[1]; d0[3] = p1; d1[0] = - p1; d2[3] = p0; d3[0] = p0; B -= 8; e -= 8; d0 += 4; d2 += 4; d1 -= 4; d3 -= 4; } } temp_free(f,buf2); temp_alloc_restore(f,save_point); }
#if 0
void inverse_mdct_naive(float *buffer, int n) { float s; float A[1 << 12], B[1 << 12], C[1 << 11]; int i,k,k2,k4, n2 = n >> 1, n4 = n >> 2, n8 = n >> 3, l; int n3_4 = n - n4, ld; float u[1 << 13], X[1 << 13], v[1 << 13], w[1 << 13]; for (k=k2=0; k < n4; ++k,k2+=2) { A[k2 ] = (float) cos(4*k*M_PI/n); A[k2+1] = (float) -sin(4*k*M_PI/n); B[k2 ] = (float) cos((k2+1)*M_PI/n/2); B[k2+1] = (float) sin((k2+1)*M_PI/n/2); } for (k=k2=0; k < n8; ++k,k2+=2) { C[k2 ] = (float) cos(2*(k2+1)*M_PI/n); C[k2+1] = (float) -sin(2*(k2+1)*M_PI/n); } for (k=0; k < n2; ++k) u[k] = buffer[k]; for ( ; k < n ; ++k) u[k] = -buffer[n - k - 1]; for (k=k2=k4=0; k < n4; k+=1, k2+=2, k4+=4) { v[n-k4-1] = (u[k4] - u[n-k4-1]) * A[k2] - (u[k4+2] - u[n-k4-3])*A[k2+1]; v[n-k4-3] = (u[k4] - u[n-k4-1]) * A[k2+1] + (u[k4+2] - u[n-k4-3])*A[k2]; } for (k=k4=0; k < n8; k+=1, k4+=4) { w[n2+3+k4] = v[n2+3+k4] + v[k4+3]; w[n2+1+k4] = v[n2+1+k4] + v[k4+1]; w[k4+3] = (v[n2+3+k4] - v[k4+3])*A[n2-4-k4] - (v[n2+1+k4]-v[k4+1])*A[n2-3-k4]; w[k4+1] = (v[n2+1+k4] - v[k4+1])*A[n2-4-k4] + (v[n2+3+k4]-v[k4+3])*A[n2-3-k4]; } ld = _vc_98fda42c(n) - 1; for (l=0; l < ld-3; ++l) { int k0 = n >> (l+2), k1 = 1 << (l+3); int rlim = n >> (l+4), r4, r; int s2lim = 1 << (l+2), s2; for (r=r4=0; r < rlim; r4+=4,++r) { for (s2=0; s2 < s2lim; s2+=2) { u[n-1-k0*s2-r4] = w[n-1-k0*s2-r4] + w[n-1-k0*(s2+1)-r4]; u[n-3-k0*s2-r4] = w[n-3-k0*s2-r4] + w[n-3-k0*(s2+1)-r4]; u[n-1-k0*(s2+1)-r4] = (w[n-1-k0*s2-r4] - w[n-1-k0*(s2+1)-r4]) * A[r*k1] - (w[n-3-k0*s2-r4] - w[n-3-k0*(s2+1)-r4]) * A[r*k1+1]; u[n-3-k0*(s2+1)-r4] = (w[n-3-k0*s2-r4] - w[n-3-k0*(s2+1)-r4]) * A[r*k1] + (w[n-1-k0*s2-r4] - w[n-1-k0*(s2+1)-r4]) * A[r*k1+1]; } } if (l+1 < ld-3) { memcpy(w, u, sizeof(u)); } } for (i=0; i < n8; ++i) { int j = _vc_7f6d13b2(i) >> (32-ld+3); assert(j < n8); if (i == j) { int i8 = i << 3; v[i8+1] = u[i8+1]; v[i8+3] = u[i8+3]; v[i8+5] = u[i8+5]; v[i8+7] = u[i8+7]; } else if (i < j) { int i8 = i << 3, j8 = j << 3; v[j8+1] = u[i8+1], v[i8+1] = u[j8 + 1]; v[j8+3] = u[i8+3], v[i8+3] = u[j8 + 3]; v[j8+5] = u[i8+5], v[i8+5] = u[j8 + 5]; v[j8+7] = u[i8+7], v[i8+7] = u[j8 + 7]; } } for (k=0; k < n2; ++k) { w[k] = v[k*2+1]; } for (k=k2=k4=0; k < n8; ++k, k2 += 2, k4 += 4) { u[n-1-k2] = w[k4]; u[n-2-k2] = w[k4+1]; u[n3_4 - 1 - k2] = w[k4+2]; u[n3_4 - 2 - k2] = w[k4+3]; } for (k=k2=0; k < n8; ++k, k2 += 2) { v[n2 + k2 ] = ( u[n2 + k2] + u[n-2-k2] + C[k2+1]*(u[n2+k2]-u[n-2-k2]) + C[k2]*(u[n2+k2+1]+u[n-2-k2+1]))/2; v[n-2 - k2] = ( u[n2 + k2] + u[n-2-k2] - C[k2+1]*(u[n2+k2]-u[n-2-k2]) - C[k2]*(u[n2+k2+1]+u[n-2-k2+1]))/2; v[n2+1+ k2] = ( u[n2+1+k2] - u[n-1-k2] + C[k2+1]*(u[n2+1+k2]+u[n-1-k2]) - C[k2]*(u[n2+k2]-u[n-2-k2]))/2; v[n-1 - k2] = (-u[n2+1+k2] + u[n-1-k2] + C[k2+1]*(u[n2+1+k2]+u[n-1-k2]) - C[k2]*(u[n2+k2]-u[n-2-k2]))/2; } for (k=k2=0; k < n4; ++k,k2 += 2) { X[k] = v[k2+n2]*B[k2 ] + v[k2+1+n2]*B[k2+1]; X[n2-1-k] = v[k2+n2]*B[k2+1] - v[k2+1+n2]*B[k2 ]; } s = 0.5; for (i=0; i < n4 ; ++i) buffer[i] = s * X[i+n4]; for ( ; i < n3_4; ++i) buffer[i] = -s * X[n3_4 - i - 1]; for ( ; i < n ; ++i) buffer[i] = -s * X[i - n3_4]; }
#endif
static float *get_window(_Vcs *f, int len) { len <<= 1; if (len == f->blocksize_0) return f->window[0]; if (len == f->blocksize_1) return f->window[1]; return NULL; }
#ifndef VORBISCOMPILER_NO_DEFER_FLR
typedef int16 YTYPE;
#else
typedef int YTYPE;
#endif
static int _vc_f40845f8(_Vcs *f, _VcMp *map, int i, int n, float *target, YTYPE *finalY, uint8 *step2_flag) { int n2 = n >> 1; int s = map->chan[i].mux, floor; floor = map->submap_floor[s]; if (f->floor_types[floor] == 0) { return _vc_11f9578d(f, VC_BAD_STREAM); } else { _VcF1 *g = &f->floor_config[floor].floor1; int j,q; int lx = 0, ly = finalY[0] * g->floor1_multiplier; for (q=1; q < g->values; ++q) { j = g->sorted_order[q];
#ifndef VORBISCOMPILER_NO_DEFER_FLR
VC_UNUSED(step2_flag); if (finalY[j] >= 0)
#else
if (step2_flag[j])
#endif
{ int hy = finalY[j] * g->floor1_multiplier; int hx = g->Xlist[j]; if (lx != hx) _vc_da904faf(target, lx,ly, hx,hy, n2); CHECK(f); lx = hx, ly = hy; } } if (lx < n2) { for (j=lx; j < n2; ++j) LINE_OP(target[j], inverse_db_table[ly]); CHECK(f); } } return TRUE; }
static int _vc_2856d2f3(_Vcs *f, int *p_left_start, int *p_left_end, int *p_right_start, int *p_right_end, int *mode) { _VcMd *m; int i, n, prev, next, window_center; f->channel_buffer_start = f->channel_buffer_end = 0; retry: if (f->eof) return FALSE; if (!_vc_dace8834(f)) return FALSE; if (_vc_a7ba7315(f,1) != 0) { if (_VC_PUSH(f)) return _vc_11f9578d(f,VC_BAD_PKT); while (EOP != _vc_e113cb67(f)); goto retry; } if (f->alloc.alloc_buffer) assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset); i = _vc_a7ba7315(f, _vc_98fda42c(f->mode_count-1)); if (i == EOP) return FALSE; if (i >= f->mode_count) return FALSE; *mode = i; m = f->mode_config + i; if (m->blockflag) { n = f->blocksize_1; prev = _vc_a7ba7315(f,1); next = _vc_a7ba7315(f,1); } else { prev = next = 0; n = f->blocksize_0; } window_center = n >> 1; if (m->blockflag && !prev) { *p_left_start = (n - f->blocksize_0) >> 2; *p_left_end = (n + f->blocksize_0) >> 2; } else { *p_left_start = 0; *p_left_end = window_center; } if (m->blockflag && !next) { *p_right_start = (n*3 - f->blocksize_0) >> 2; *p_right_end = (n*3 + f->blocksize_0) >> 2; } else { *p_right_start = window_center; *p_right_end = n; } return TRUE; }
static int _vc_5c100599(_Vcs *f, int *len, _VcMd *m, int left_start, int left_end, int right_start, int right_end, int *p_left) { _VcMp *map; int i,j,k,n,n2; int zero_channel[256]; int really_zero_channel[256]; VC_UNUSED(left_end); n = f->blocksize[m->blockflag]; map = &f->mapping[m->mapping]; n2 = n >> 1; CHECK(f); for (i=0; i < f->channels; ++i) { int s = map->chan[i].mux, floor; zero_channel[i] = FALSE; floor = map->submap_floor[s]; if (f->floor_types[floor] == 0) { return _vc_11f9578d(f, VC_BAD_STREAM); } else { _VcF1 *g = &f->floor_config[floor].floor1; if (_vc_a7ba7315(f, 1)) { short *finalY; uint8 step2_flag[256]; static int range_list[4] = { 256, 128, 86, 64 }; int range = range_list[g->floor1_multiplier-1]; int offset = 2; finalY = f->finalY[i]; finalY[0] = _vc_a7ba7315(f, _vc_98fda42c(range)-1); finalY[1] = _vc_a7ba7315(f, _vc_98fda42c(range)-1); for (j=0; j < g->partitions; ++j) { int pclass = g->partition_class_list[j]; int cdim = g->class_dimensions[pclass]; int cbits = g->class_subclasses[pclass]; int csub = (1 << cbits)-1; int cval = 0; if (cbits) { _VcCb *c = f->codebooks + g->class_masterbooks[pclass]; DECODE(cval,f,c); } for (k=0; k < cdim; ++k) { int book = g->subclass_books[pclass][cval & csub]; cval = cval >> cbits; if (book >= 0) { int temp; _VcCb *c = f->codebooks + book; DECODE(temp,f,c); finalY[offset++] = temp; } else finalY[offset++] = 0; } } if (f->valid_bits == INVALID_BITS) goto error; step2_flag[0] = step2_flag[1] = 1; for (j=2; j < g->values; ++j) { int low, high, pred, highroom, lowroom, room, val; low = g->neighbors[j][0]; high = g->neighbors[j][1]; pred = _vc_3f512384(g->Xlist[j], g->Xlist[low], g->Xlist[high], finalY[low], finalY[high]); val = finalY[j]; highroom = range - pred; lowroom = pred; if (highroom < lowroom) room = highroom * 2; else room = lowroom * 2; if (val) { step2_flag[low] = step2_flag[high] = 1; step2_flag[j] = 1; if (val >= room) if (highroom > lowroom) finalY[j] = val - lowroom + pred; else finalY[j] = pred - val + highroom - 1; else if (val & 1) finalY[j] = pred - ((val+1)>>1); else finalY[j] = pred + (val>>1); } else { step2_flag[j] = 0; finalY[j] = pred; } }
#ifdef VORBISCOMPILER_NO_DEFER_FLR
_vc_f40845f8(f, map, i, n, f->floor_buffers[i], finalY, step2_flag);
#else
for (j=0; j < g->values; ++j) { if (!step2_flag[j]) finalY[j] = -1; }
#endif
} else { error: zero_channel[i] = TRUE; } } } CHECK(f); if (f->alloc.alloc_buffer) assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset); memcpy(really_zero_channel, zero_channel, sizeof(really_zero_channel[0]) * f->channels); for (i=0; i < map->coupling_steps; ++i) if (!zero_channel[map->chan[i].magnitude] || !zero_channel[map->chan[i].angle]) { zero_channel[map->chan[i].magnitude] = zero_channel[map->chan[i].angle] = FALSE; } CHECK(f); for (i=0; i < map->submaps; ++i) { float *residue_buffers[VORBISCOMPILER_MAX_CH]; int r; uint8 do_not_decode[256]; int ch = 0; for (j=0; j < f->channels; ++j) { if (map->chan[j].mux == i) { if (zero_channel[j]) { do_not_decode[ch] = TRUE; residue_buffers[ch] = NULL; } else { do_not_decode[ch] = FALSE; residue_buffers[ch] = f->channel_buffers[j]; } ++ch; } } r = map->submap_residue[i]; _vc_a67261bf(f, residue_buffers, ch, n2, r, do_not_decode); } if (f->alloc.alloc_buffer) assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset); CHECK(f); for (i = map->coupling_steps-1; i >= 0; --i) { int n2 = n >> 1; float *m = f->channel_buffers[map->chan[i].magnitude]; float *a = f->channel_buffers[map->chan[i].angle ]; for (j=0; j < n2; ++j) { float a2,m2; if (m[j] > 0) if (a[j] > 0) m2 = m[j], a2 = m[j] - a[j]; else a2 = m[j], m2 = m[j] + a[j]; else if (a[j] > 0) m2 = m[j], a2 = m[j] + a[j]; else a2 = m[j], m2 = m[j] - a[j]; m[j] = m2; a[j] = a2; } } CHECK(f);
#ifndef VORBISCOMPILER_NO_DEFER_FLR
for (i=0; i < f->channels; ++i) { if (really_zero_channel[i]) { memset(f->channel_buffers[i], 0, sizeof(*f->channel_buffers[i]) * n2); } else { _vc_f40845f8(f, map, i, n, f->channel_buffers[i], f->finalY[i], NULL); } }
#else
for (i=0; i < f->channels; ++i) { if (really_zero_channel[i]) { memset(f->channel_buffers[i], 0, sizeof(*f->channel_buffers[i]) * n2); } else { for (j=0; j < n2; ++j) f->channel_buffers[i][j] *= f->floor_buffers[i][j]; } }
#endif
CHECK(f); for (i=0; i < f->channels; ++i) _vc_87664557(f->channel_buffers[i], n, f, m->blockflag); CHECK(f); _vc_b2ff18b0(f); if (f->first_decode) { f->current_loc = 0u - n2; f->discard_samples_deferred = n - right_end; f->current_loc_valid = TRUE; f->first_decode = FALSE; } else if (f->discard_samples_deferred) { if (f->discard_samples_deferred >= right_start - left_start) { f->discard_samples_deferred -= (right_start - left_start); left_start = right_start; *p_left = left_start; } else { left_start += f->discard_samples_deferred; *p_left = left_start; f->discard_samples_deferred = 0; } } else if (f->previous_length == 0 && f->current_loc_valid) { } if (f->last_seg_which == f->end_seg_with_known_loc) { if (f->current_loc_valid && (f->page_flag & _VC_PF_LAST)) { uint32 current_end = f->known_loc_for_packet; if (current_end < f->current_loc + (right_end-left_start)) { if (current_end < f->current_loc) { *len = 0; } else { *len = current_end - f->current_loc; } *len += left_start; if (*len > right_end) *len = right_end; f->current_loc += *len; return TRUE; } } f->current_loc = f->known_loc_for_packet - (n2-left_start); f->current_loc_valid = TRUE; } if (f->current_loc_valid) f->current_loc += (right_start - left_start); if (f->alloc.alloc_buffer) assert(f->alloc.alloc_buffer_length_in_bytes == f->temp_offset); *len = right_end; CHECK(f); return TRUE; }
static int _vc_7256c65e(_Vcs *f, int *len, int *p_left, int *p_right) { int mode, left_end, right_end; if (!_vc_2856d2f3(f, p_left, &left_end, p_right, &right_end, &mode)) return 0; return _vc_5c100599(f, len, f->mode_config + mode, *p_left, left_end, *p_right, right_end, p_left); }
static int _vc_98c006bf(vc_stream *f, int len, int left, int right) { int prev,i,j; if (f->previous_length) { int i,j, n = f->previous_length; float *w = get_window(f, n); if (w == NULL) return 0; for (i=0; i < f->channels; ++i) { for (j=0; j < n; ++j) f->channel_buffers[i][left+j] = f->channel_buffers[i][left+j]*w[ j] + f->previous_window[i][ j]*w[n-1-j]; } } prev = f->previous_length; f->previous_length = len - right; for (i=0; i < f->channels; ++i) for (j=0; right+j < len; ++j) f->previous_window[i][j] = f->channel_buffers[i][right+j]; if (!prev) return 0; if (len < right) right = len; f->samples_output += right-left; return right - left; }
static int _vc_2a5f6090(vc_stream *f) { int len, right, left, res; res = _vc_7256c65e(f, &len, &left, &right); if (res) _vc_98c006bf(f, len, left, right); return res; }
#ifndef VORBISCOMPILER_NO_PUSH
static int _vc_7ef58f6d(vc_stream *f) { int s = f->next_seg, first = TRUE; uint8 *p = f->stream; if (s != -1) { for (; s < f->segment_count; ++s) { p += f->segments[s]; if (f->segments[s] < 255) break; } if (s == f->segment_count) s = -1; if (p > f->stream_end) return _vc_11f9578d(f, VC_NEED_DATA); first = FALSE; } for (; s == -1;) { uint8 *q; int n; if (p + 26 >= f->stream_end) return _vc_11f9578d(f, VC_NEED_DATA); if (memcmp(p, _vc_ogg_hdr, 4)) return _vc_11f9578d(f, VC_BAD_STREAM); if (p[4] != 0) return _vc_11f9578d(f, VC_BAD_STREAM); if (first) { if (f->previous_length) if ((p[5] & _VC_PF_CONT)) return _vc_11f9578d(f, VC_BAD_STREAM); } else { if (!(p[5] & _VC_PF_CONT)) return _vc_11f9578d(f, VC_BAD_STREAM); } n = p[26]; q = p+27; p = q + n; if (p > f->stream_end) return _vc_11f9578d(f, VC_NEED_DATA); for (s=0; s < n; ++s) { p += q[s]; if (q[s] < 255) break; } if (s == n) s = -1; if (p > f->stream_end) return _vc_11f9578d(f, VC_NEED_DATA); first = FALSE; } return TRUE; }
#endif
static int _vc_2fc069f2(_Vcs *f) { uint8 header[6], x,y; int len,i,j,k, max_submaps = 0; int longest_floorlist=0; f->first_decode = TRUE; if (!_vc_e7208f0c(f)) return FALSE; if (!(f->page_flag & _VC_PF_FIRST)) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (f->page_flag & _VC_PF_LAST) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (f->page_flag & _VC_PF_CONT) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (f->segment_count != 1) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (f->segments[0] != 30) { if (f->segments[0] == 64 && _vc_fc62ee61(f, header, 6) && header[0] == 'f' && header[1] == 'i' && header[2] == 's' && header[3] == 'h' && header[4] == 'e' && header[5] == 'a' && _vc_90d1fe2e(f) == 'd' && _vc_90d1fe2e(f) == '\0') return _vc_11f9578d(f, VC_OGG_SKEL); else return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); } if (_vc_90d1fe2e(f) != VC_PKT_ID) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (!_vc_fc62ee61(f, header, 6)) return _vc_11f9578d(f, VC_EOF); if (!_vc_d1210c1d(header)) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (_vc_a8816410(f) != 0) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); f->channels = _vc_90d1fe2e(f); if (!f->channels) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (f->channels > VORBISCOMPILER_MAX_CH) return _vc_11f9578d(f, VC_TOO_MANY_CH); f->sample_rate = _vc_a8816410(f); if (!f->sample_rate) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); _vc_a8816410(f); _vc_a8816410(f); _vc_a8816410(f); x = _vc_90d1fe2e(f); { int log0,log1; log0 = x & 15; log1 = x >> 4; f->blocksize_0 = 1 << log0; f->blocksize_1 = 1 << log1; if (log0 < 6 || log0 > 13) return _vc_11f9578d(f, VC_BAD_SETUP); if (log1 < 6 || log1 > 13) return _vc_11f9578d(f, VC_BAD_SETUP); if (log0 > log1) return _vc_11f9578d(f, VC_BAD_SETUP); } x = _vc_90d1fe2e(f); if (!(x & 1)) return _vc_11f9578d(f, VC_BAD_FIRST_PAGE); if (!_vc_e7208f0c(f)) return FALSE; if (!_vc_7cf885ee(f)) return FALSE; if (!_vc_4c441ef3(f)) return FALSE; if (_vc_e113cb67(f) != VC_PKT_COMMENT) return _vc_11f9578d(f, VC_BAD_SETUP); for (i=0; i < 6; ++i) header[i] = _vc_e113cb67(f); if (!_vc_d1210c1d(header)) return _vc_11f9578d(f, VC_BAD_SETUP); len = _vc_a8b1002a(f); f->vendor = (char*)setup_malloc(f, sizeof(char) * (len+1)); if (f->vendor == NULL) return _vc_11f9578d(f, VC_OOM); for(i=0; i < len; ++i) { f->vendor[i] = _vc_e113cb67(f); } f->vendor[len] = (char)'\0'; f->comment_list_length = _vc_a8b1002a(f); f->comment_list = NULL; if (f->comment_list_length > 0) { f->comment_list = (char**) setup_malloc(f, sizeof(char*) * (f->comment_list_length)); if (f->comment_list == NULL) return _vc_11f9578d(f, VC_OOM); } for(i=0; i < f->comment_list_length; ++i) { len = _vc_a8b1002a(f); f->comment_list[i] = (char*)setup_malloc(f, sizeof(char) * (len+1)); if (f->comment_list[i] == NULL) return _vc_11f9578d(f, VC_OOM); for(j=0; j < len; ++j) { f->comment_list[i][j] = _vc_e113cb67(f); } f->comment_list[i][len] = (char)'\0'; } x = _vc_e113cb67(f); if (!(x & 1)) return _vc_11f9578d(f, VC_BAD_SETUP); _vc_c7e16815(f, f->bytes_in_seg); f->bytes_in_seg = 0; do { len = _vc_4c441ef3(f); _vc_c7e16815(f, len); f->bytes_in_seg = 0; } while (len); if (!_vc_7cf885ee(f)) return FALSE;
#ifndef VORBISCOMPILER_NO_PUSH
if (_VC_PUSH(f)) { if (!_vc_7ef58f6d(f)) { if (f->error == VC_BAD_STREAM) f->error = VC_BAD_SETUP; return FALSE; } }
#endif
_vc_b66b74bf(); if (_vc_e113cb67(f) != VC_PKT_SETUP) return _vc_11f9578d(f, VC_BAD_SETUP); for (i=0; i < 6; ++i) header[i] = _vc_e113cb67(f); if (!_vc_d1210c1d(header)) return _vc_11f9578d(f, VC_BAD_SETUP); f->codebook_count = _vc_a7ba7315(f,8) + 1; f->codebooks = (_VcCb *) setup_malloc(f, sizeof(*f->codebooks) * f->codebook_count); if (f->codebooks == NULL) return _vc_11f9578d(f, VC_OOM); memset(f->codebooks, 0, sizeof(*f->codebooks) * f->codebook_count); for (i=0; i < f->codebook_count; ++i) { uint32 *values; int ordered, sorted_count; int total=0; uint8 *lengths; _VcCb *c = f->codebooks+i; CHECK(f); x = _vc_a7ba7315(f, 8); if (x != 0x42) return _vc_11f9578d(f, VC_BAD_SETUP); x = _vc_a7ba7315(f, 8); if (x != 0x43) return _vc_11f9578d(f, VC_BAD_SETUP); x = _vc_a7ba7315(f, 8); if (x != 0x56) return _vc_11f9578d(f, VC_BAD_SETUP); x = _vc_a7ba7315(f, 8); c->dimensions = (_vc_a7ba7315(f, 8)<<8) + x; x = _vc_a7ba7315(f, 8); y = _vc_a7ba7315(f, 8); c->entries = (_vc_a7ba7315(f, 8)<<16) + (y<<8) + x; ordered = _vc_a7ba7315(f,1); c->sparse = ordered ? 0 : _vc_a7ba7315(f,1); if (c->dimensions == 0 && c->entries != 0) return _vc_11f9578d(f, VC_BAD_SETUP); if (c->sparse) lengths = (uint8 *) setup_temp_malloc(f, c->entries); else lengths = c->codeword_lengths = (uint8 *) setup_malloc(f, c->entries); if (!lengths) return _vc_11f9578d(f, VC_OOM); if (ordered) { int current_entry = 0; int current_length = _vc_a7ba7315(f,5) + 1; while (current_entry < c->entries) { int limit = c->entries - current_entry; int n = _vc_a7ba7315(f, _vc_98fda42c(limit)); if (current_length >= 32) return _vc_11f9578d(f, VC_BAD_SETUP); if (current_entry + n > (int) c->entries) { return _vc_11f9578d(f, VC_BAD_SETUP); } memset(lengths + current_entry, current_length, n); current_entry += n; ++current_length; } } else { for (j=0; j < c->entries; ++j) { int present = c->sparse ? _vc_a7ba7315(f,1) : 1; if (present) { lengths[j] = _vc_a7ba7315(f, 5) + 1; ++total; if (lengths[j] == 32) return _vc_11f9578d(f, VC_BAD_SETUP); } else { lengths[j] = _VC_NC; } } } if (c->sparse && total >= c->entries >> 2) { if (c->entries > (int) f->setup_temp_memory_required) f->setup_temp_memory_required = c->entries; c->codeword_lengths = (uint8 *) setup_malloc(f, c->entries); if (c->codeword_lengths == NULL) return _vc_11f9578d(f, VC_OOM); memcpy(c->codeword_lengths, lengths, c->entries); _vc_8c8cdbe9(f, lengths, c->entries); lengths = c->codeword_lengths; c->sparse = 0; } if (c->sparse) { sorted_count = total; } else { sorted_count = 0;
#ifndef VORBISCOMPILER_NO_HUFF_BIN
for (j=0; j < c->entries; ++j) if (lengths[j] > VORBISCOMPILER_HUFF_BITS && lengths[j] != _VC_NC) ++sorted_count;
#endif
} c->sorted_entries = sorted_count; values = NULL; CHECK(f); if (!c->sparse) { c->codewords = (uint32 *) setup_malloc(f, sizeof(c->codewords[0]) * c->entries); if (!c->codewords) return _vc_11f9578d(f, VC_OOM); } else { unsigned int size; if (c->sorted_entries) { c->codeword_lengths = (uint8 *) setup_malloc(f, c->sorted_entries); if (!c->codeword_lengths) return _vc_11f9578d(f, VC_OOM); c->codewords = (uint32 *) setup_temp_malloc(f, sizeof(*c->codewords) * c->sorted_entries); if (!c->codewords) return _vc_11f9578d(f, VC_OOM); values = (uint32 *) setup_temp_malloc(f, sizeof(*values) * c->sorted_entries); if (!values) return _vc_11f9578d(f, VC_OOM); } size = c->entries + (sizeof(*c->codewords) + sizeof(*values)) * c->sorted_entries; if (size > f->setup_temp_memory_required) f->setup_temp_memory_required = size; } if (!_vc_561d1c9e(c, lengths, c->entries, values)) { if (c->sparse) _vc_8c8cdbe9(f, values, 0); return _vc_11f9578d(f, VC_BAD_SETUP); } if (c->sorted_entries) { c->sorted_codewords = (uint32 *) setup_malloc(f, sizeof(*c->sorted_codewords) * (c->sorted_entries+1)); if (c->sorted_codewords == NULL) return _vc_11f9578d(f, VC_OOM); c->sorted_values = ( int *) setup_malloc(f, sizeof(*c->sorted_values ) * (c->sorted_entries+1)); if (c->sorted_values == NULL) return _vc_11f9578d(f, VC_OOM); ++c->sorted_values; c->sorted_values[-1] = -1; _vc_b27db06e(c, lengths, values); } if (c->sparse) { _vc_8c8cdbe9(f, values, sizeof(*values)*c->sorted_entries); _vc_8c8cdbe9(f, c->codewords, sizeof(*c->codewords)*c->sorted_entries); _vc_8c8cdbe9(f, lengths, c->entries); c->codewords = NULL; } _vc_6c87b769(c); CHECK(f); c->lookup_type = _vc_a7ba7315(f, 4); if (c->lookup_type > 2) return _vc_11f9578d(f, VC_BAD_SETUP); if (c->lookup_type > 0) { uint16 *mults; c->minimum_value = _vc_3fd60b95(_vc_a7ba7315(f, 32)); c->delta_value = _vc_3fd60b95(_vc_a7ba7315(f, 32)); c->value_bits = _vc_a7ba7315(f, 4)+1; c->sequence_p = _vc_a7ba7315(f,1); if (c->lookup_type == 1) { int values = _vc_3e2962c5(c->entries, c->dimensions); if (values < 0) return _vc_11f9578d(f, VC_BAD_SETUP); c->lookup_values = (uint32) values; } else { c->lookup_values = c->entries * c->dimensions; } if (c->lookup_values == 0) return _vc_11f9578d(f, VC_BAD_SETUP); mults = (uint16 *) setup_temp_malloc(f, sizeof(mults[0]) * c->lookup_values); if (mults == NULL) return _vc_11f9578d(f, VC_OOM); for (j=0; j < (int) c->lookup_values; ++j) { int q = _vc_a7ba7315(f, c->value_bits); if (q == EOP) { _vc_8c8cdbe9(f,mults,sizeof(mults[0])*c->lookup_values); return _vc_11f9578d(f, VC_BAD_SETUP); } mults[j] = q; }
#ifndef VORBISCOMPILER_DIV_CODEBOOK
if (c->lookup_type == 1) { int len, sparse = c->sparse; float last=0; if (sparse) { if (c->sorted_entries == 0) goto skip; c->multiplicands = (_VcCt *) setup_malloc(f, sizeof(c->multiplicands[0]) * c->sorted_entries * c->dimensions); } else c->multiplicands = (_VcCt *) setup_malloc(f, sizeof(c->multiplicands[0]) * c->entries * c->dimensions); if (c->multiplicands == NULL) { _vc_8c8cdbe9(f,mults,sizeof(mults[0])*c->lookup_values); return _vc_11f9578d(f, VC_OOM); } len = sparse ? c->sorted_entries : c->entries; for (j=0; j < len; ++j) { unsigned int z = sparse ? c->sorted_values[j] : j; unsigned int div=1; for (k=0; k < c->dimensions; ++k) { int off = (z / div) % c->lookup_values; float val = mults[off]*c->delta_value + c->minimum_value + last; c->multiplicands[j*c->dimensions + k] = val; if (c->sequence_p) last = val; if (k+1 < c->dimensions) { if (div > UINT_MAX / (unsigned int) c->lookup_values) { _vc_8c8cdbe9(f, mults,sizeof(mults[0])*c->lookup_values); return _vc_11f9578d(f, VC_BAD_SETUP); } div *= c->lookup_values; } } } c->lookup_type = 2; } else
#endif
{ float last=0; CHECK(f); c->multiplicands = (_VcCt *) setup_malloc(f, sizeof(c->multiplicands[0]) * c->lookup_values); if (c->multiplicands == NULL) { _vc_8c8cdbe9(f, mults,sizeof(mults[0])*c->lookup_values); return _vc_11f9578d(f, VC_OOM); } for (j=0; j < (int) c->lookup_values; ++j) { float val = mults[j] * c->delta_value + c->minimum_value + last; c->multiplicands[j] = val; if (c->sequence_p) last = val; } }
#ifndef VORBISCOMPILER_DIV_CODEBOOK
skip:;
#endif
_vc_8c8cdbe9(f, mults, sizeof(mults[0])*c->lookup_values); CHECK(f); } CHECK(f); } x = _vc_a7ba7315(f, 6) + 1; for (i=0; i < x; ++i) { uint32 z = _vc_a7ba7315(f, 16); if (z != 0) return _vc_11f9578d(f, VC_BAD_SETUP); } f->floor_count = _vc_a7ba7315(f, 6)+1; f->floor_config = (_VcFl *) setup_malloc(f, f->floor_count * sizeof(*f->floor_config)); if (f->floor_config == NULL) return _vc_11f9578d(f, VC_OOM); for (i=0; i < f->floor_count; ++i) { f->floor_types[i] = _vc_a7ba7315(f, 16); if (f->floor_types[i] > 1) return _vc_11f9578d(f, VC_BAD_SETUP); if (f->floor_types[i] == 0) { _VcF0 *g = &f->floor_config[i].floor0; g->order = _vc_a7ba7315(f,8); g->rate = _vc_a7ba7315(f,16); g->bark_map_size = _vc_a7ba7315(f,16); g->amplitude_bits = _vc_a7ba7315(f,6); g->amplitude_offset = _vc_a7ba7315(f,8); g->number_of_books = _vc_a7ba7315(f,4) + 1; for (j=0; j < g->number_of_books; ++j) g->book_list[j] = _vc_a7ba7315(f,8); return _vc_11f9578d(f, VC_UNSUPPORTED); } else { _VcFlOrd p[31*8+2]; _VcF1 *g = &f->floor_config[i].floor1; int max_class = -1; g->partitions = _vc_a7ba7315(f, 5); for (j=0; j < g->partitions; ++j) { g->partition_class_list[j] = _vc_a7ba7315(f, 4); if (g->partition_class_list[j] > max_class) max_class = g->partition_class_list[j]; } for (j=0; j <= max_class; ++j) { g->class_dimensions[j] = _vc_a7ba7315(f, 3)+1; g->class_subclasses[j] = _vc_a7ba7315(f, 2); if (g->class_subclasses[j]) { g->class_masterbooks[j] = _vc_a7ba7315(f, 8); if (g->class_masterbooks[j] >= f->codebook_count) return _vc_11f9578d(f, VC_BAD_SETUP); } for (k=0; k < 1 << g->class_subclasses[j]; ++k) { g->subclass_books[j][k] = (int16)_vc_a7ba7315(f,8)-1; if (g->subclass_books[j][k] >= f->codebook_count) return _vc_11f9578d(f, VC_BAD_SETUP); } } g->floor1_multiplier = _vc_a7ba7315(f,2)+1; g->rangebits = _vc_a7ba7315(f,4); g->Xlist[0] = 0; g->Xlist[1] = 1 << g->rangebits; g->values = 2; for (j=0; j < g->partitions; ++j) { int c = g->partition_class_list[j]; for (k=0; k < g->class_dimensions[c]; ++k) { g->Xlist[g->values] = _vc_a7ba7315(f, g->rangebits); ++g->values; } } for (j=0; j < g->values; ++j) { p[j].x = g->Xlist[j]; p[j].id = j; } qsort(p, g->values, sizeof(p[0]), _vc_efdc5df4); for (j=0; j < g->values-1; ++j) if (p[j].x == p[j+1].x) return _vc_11f9578d(f, VC_BAD_SETUP); for (j=0; j < g->values; ++j) g->sorted_order[j] = (uint8) p[j].id; for (j=2; j < g->values; ++j) { int low = 0,hi = 0; _vc_a24fee51(g->Xlist, j, &low,&hi); g->neighbors[j][0] = low; g->neighbors[j][1] = hi; } if (g->values > longest_floorlist) longest_floorlist = g->values; } } f->residue_count = _vc_a7ba7315(f, 6)+1; f->residue_config = (_VcRs *) setup_malloc(f, f->residue_count * sizeof(f->residue_config[0])); if (f->residue_config == NULL) return _vc_11f9578d(f, VC_OOM); memset(f->residue_config, 0, f->residue_count * sizeof(f->residue_config[0])); for (i=0; i < f->residue_count; ++i) { uint8 residue_cascade[64]; _VcRs *r = f->residue_config+i; f->residue_types[i] = _vc_a7ba7315(f, 16); if (f->residue_types[i] > 2) return _vc_11f9578d(f, VC_BAD_SETUP); r->begin = _vc_a7ba7315(f, 24); r->end = _vc_a7ba7315(f, 24); if (r->end < r->begin) return _vc_11f9578d(f, VC_BAD_SETUP); r->part_size = _vc_a7ba7315(f,24)+1; r->classifications = _vc_a7ba7315(f,6)+1; r->classbook = _vc_a7ba7315(f,8); if (r->classbook >= f->codebook_count) return _vc_11f9578d(f, VC_BAD_SETUP); for (j=0; j < r->classifications; ++j) { uint8 high_bits=0; uint8 low_bits=_vc_a7ba7315(f,3); if (_vc_a7ba7315(f,1)) high_bits = _vc_a7ba7315(f,5); residue_cascade[j] = high_bits*8 + low_bits; } r->residue_books = (short (*)[8]) setup_malloc(f, sizeof(r->residue_books[0]) * r->classifications); if (r->residue_books == NULL) return _vc_11f9578d(f, VC_OOM); for (j=0; j < r->classifications; ++j) { for (k=0; k < 8; ++k) { if (residue_cascade[j] & (1 << k)) { r->residue_books[j][k] = _vc_a7ba7315(f, 8); if (r->residue_books[j][k] >= f->codebook_count) return _vc_11f9578d(f, VC_BAD_SETUP); } else { r->residue_books[j][k] = -1; } } } r->classdata = (uint8 **) setup_malloc(f, sizeof(*r->classdata) * f->codebooks[r->classbook].entries); if (!r->classdata) return _vc_11f9578d(f, VC_OOM); memset(r->classdata, 0, sizeof(*r->classdata) * f->codebooks[r->classbook].entries); for (j=0; j < f->codebooks[r->classbook].entries; ++j) { int classwords = f->codebooks[r->classbook].dimensions; int temp = j; r->classdata[j] = (uint8 *) setup_malloc(f, sizeof(r->classdata[j][0]) * classwords); if (r->classdata[j] == NULL) return _vc_11f9578d(f, VC_OOM); for (k=classwords-1; k >= 0; --k) { r->classdata[j][k] = temp % r->classifications; temp /= r->classifications; } } } f->mapping_count = _vc_a7ba7315(f,6)+1; f->mapping = (_VcMp *) setup_malloc(f, f->mapping_count * sizeof(*f->mapping)); if (f->mapping == NULL) return _vc_11f9578d(f, VC_OOM); memset(f->mapping, 0, f->mapping_count * sizeof(*f->mapping)); for (i=0; i < f->mapping_count; ++i) { _VcMp *m = f->mapping + i; int mapping_type = _vc_a7ba7315(f,16); if (mapping_type != 0) return _vc_11f9578d(f, VC_BAD_SETUP); m->chan = (_VcMc *) setup_malloc(f, f->channels * sizeof(*m->chan)); if (m->chan == NULL) return _vc_11f9578d(f, VC_OOM); if (_vc_a7ba7315(f,1)) m->submaps = _vc_a7ba7315(f,4)+1; else m->submaps = 1; if (m->submaps > max_submaps) max_submaps = m->submaps; if (_vc_a7ba7315(f,1)) { m->coupling_steps = _vc_a7ba7315(f,8)+1; if (m->coupling_steps > f->channels) return _vc_11f9578d(f, VC_BAD_SETUP); for (k=0; k < m->coupling_steps; ++k) { m->chan[k].magnitude = _vc_a7ba7315(f, _vc_98fda42c(f->channels-1)); m->chan[k].angle = _vc_a7ba7315(f, _vc_98fda42c(f->channels-1)); if (m->chan[k].magnitude >= f->channels) return _vc_11f9578d(f, VC_BAD_SETUP); if (m->chan[k].angle >= f->channels) return _vc_11f9578d(f, VC_BAD_SETUP); if (m->chan[k].magnitude == m->chan[k].angle) return _vc_11f9578d(f, VC_BAD_SETUP); } } else m->coupling_steps = 0; if (_vc_a7ba7315(f,2)) return _vc_11f9578d(f, VC_BAD_SETUP); if (m->submaps > 1) { for (j=0; j < f->channels; ++j) { m->chan[j].mux = _vc_a7ba7315(f, 4); if (m->chan[j].mux >= m->submaps) return _vc_11f9578d(f, VC_BAD_SETUP); } } else for (j=0; j < f->channels; ++j) m->chan[j].mux = 0; for (j=0; j < m->submaps; ++j) { _vc_a7ba7315(f,8); m->submap_floor[j] = _vc_a7ba7315(f,8); m->submap_residue[j] = _vc_a7ba7315(f,8); if (m->submap_floor[j] >= f->floor_count) return _vc_11f9578d(f, VC_BAD_SETUP); if (m->submap_residue[j] >= f->residue_count) return _vc_11f9578d(f, VC_BAD_SETUP); } } f->mode_count = _vc_a7ba7315(f, 6)+1; for (i=0; i < f->mode_count; ++i) { _VcMd *m = f->mode_config+i; m->blockflag = _vc_a7ba7315(f,1); m->windowtype = _vc_a7ba7315(f,16); m->transformtype = _vc_a7ba7315(f,16); m->mapping = _vc_a7ba7315(f,8); if (m->windowtype != 0) return _vc_11f9578d(f, VC_BAD_SETUP); if (m->transformtype != 0) return _vc_11f9578d(f, VC_BAD_SETUP); if (m->mapping >= f->mapping_count) return _vc_11f9578d(f, VC_BAD_SETUP); } _vc_b2ff18b0(f); f->previous_length = 0; for (i=0; i < f->channels; ++i) { f->channel_buffers[i] = (float *) setup_malloc(f, sizeof(float) * f->blocksize_1); f->previous_window[i] = (float *) setup_malloc(f, sizeof(float) * f->blocksize_1/2); f->finalY[i] = (int16 *) setup_malloc(f, sizeof(int16) * longest_floorlist); if (f->channel_buffers[i] == NULL || f->previous_window[i] == NULL || f->finalY[i] == NULL) return _vc_11f9578d(f, VC_OOM); memset(f->channel_buffers[i], 0, sizeof(float) * f->blocksize_1);
#ifdef VORBISCOMPILER_NO_DEFER_FLR
f->floor_buffers[i] = (float *) setup_malloc(f, sizeof(float) * f->blocksize_1/2); if (f->floor_buffers[i] == NULL) return _vc_11f9578d(f, VC_OOM);
#endif
} if (!_vc_b9459f6b(f, 0, f->blocksize_0)) return FALSE; if (!_vc_b9459f6b(f, 1, f->blocksize_1)) return FALSE; f->blocksize[0] = f->blocksize_0; f->blocksize[1] = f->blocksize_1;
#ifdef VORBISCOMPILER_DIV_TBL
if (integer_divide_table[1][1]==0) for (i=0; i < DIVTAB_NUMER; ++i) for (j=1; j < DIVTAB_DENOM; ++j) integer_divide_table[i][j] = i / j;
#endif
{ uint32 imdct_mem = (f->blocksize_1 * sizeof(float) >> 1); uint32 classify_mem; int i,max_part_read=0; for (i=0; i < f->residue_count; ++i) { _VcRs *r = f->residue_config + i; unsigned int actual_size = f->blocksize_1 / 2; unsigned int limit_r_begin = r->begin < actual_size ? r->begin : actual_size; unsigned int limit_r_end = r->end < actual_size ? r->end : actual_size; int n_read = limit_r_end - limit_r_begin; int part_read = n_read / r->part_size; if (part_read > max_part_read) max_part_read = part_read; }
#ifndef VORBISCOMPILER_DIV_RESIDUE
classify_mem = f->channels * (sizeof(void*) + max_part_read * sizeof(uint8 *));
#else
classify_mem = f->channels * (sizeof(void*) + max_part_read * sizeof(int *));
#endif
f->temp_memory_required = classify_mem; if (imdct_mem > f->temp_memory_required) f->temp_memory_required = imdct_mem; } if (f->alloc.alloc_buffer) { assert(f->temp_offset == f->alloc.alloc_buffer_length_in_bytes); if (f->setup_offset + sizeof(*f) + f->temp_memory_required > (unsigned) f->temp_offset) return _vc_11f9578d(f, VC_OOM); } if (f->next_seg == -1) { f->first_audio_page_offset = vc_get_file_offset(f); } else { f->first_audio_page_offset = 0; } return TRUE; }
static void _vc_65a45e77(vc_stream *p) { int i,j; _vc_ce046650(p, p->vendor); for (i=0; i < p->comment_list_length; ++i) { _vc_ce046650(p, p->comment_list[i]); } _vc_ce046650(p, p->comment_list); if (p->residue_config) { for (i=0; i < p->residue_count; ++i) { _VcRs *r = p->residue_config+i; if (r->classdata) { for (j=0; j < p->codebooks[r->classbook].entries; ++j) _vc_ce046650(p, r->classdata[j]); _vc_ce046650(p, r->classdata); } _vc_ce046650(p, r->residue_books); } } if (p->codebooks) { CHECK(p); for (i=0; i < p->codebook_count; ++i) { _VcCb *c = p->codebooks + i; _vc_ce046650(p, c->codeword_lengths); _vc_ce046650(p, c->multiplicands); _vc_ce046650(p, c->codewords); _vc_ce046650(p, c->sorted_codewords); _vc_ce046650(p, c->sorted_values ? c->sorted_values-1 : NULL); } _vc_ce046650(p, p->codebooks); } _vc_ce046650(p, p->floor_config); _vc_ce046650(p, p->residue_config); if (p->mapping) { for (i=0; i < p->mapping_count; ++i) _vc_ce046650(p, p->mapping[i].chan); _vc_ce046650(p, p->mapping); } CHECK(p); for (i=0; i < p->channels && i < VORBISCOMPILER_MAX_CH; ++i) { _vc_ce046650(p, p->channel_buffers[i]); _vc_ce046650(p, p->previous_window[i]);
#ifdef VORBISCOMPILER_NO_DEFER_FLR
_vc_ce046650(p, p->floor_buffers[i]);
#endif
_vc_ce046650(p, p->finalY[i]); } for (i=0; i < 2; ++i) { _vc_ce046650(p, p->A[i]); _vc_ce046650(p, p->B[i]); _vc_ce046650(p, p->C[i]); _vc_ce046650(p, p->window[i]); _vc_ce046650(p, p->bit_reverse[i]); }
#ifndef VORBISCOMPILER_NO_STDIO
if (p->close_on_free) fclose(p->f);
#endif
}
void vc_close(vc_stream *p) { if (p == NULL) return; _vc_65a45e77(p); _vc_ce046650(p,p); }
static void _vc_48482100(vc_stream *p, const vc_alloc *z) { memset(p, 0, sizeof(*p)); if (z) { p->alloc = *z; p->alloc.alloc_buffer_length_in_bytes &= ~7; p->temp_offset = p->alloc.alloc_buffer_length_in_bytes; } p->eof = 0; p->error = VC_OK; p->stream = NULL; p->codebooks = NULL; p->page_crc_tests = -1;
#ifndef VORBISCOMPILER_NO_STDIO
p->close_on_free = FALSE; p->f = NULL;
#endif
}
int vc_get_sample_offset(vc_stream *f) { if (f->current_loc_valid) return f->current_loc; else return -1; }
vc_stream_info vc_get_info(vc_stream *f) { vc_stream_info d; d.channels = f->channels; d.sample_rate = f->sample_rate; d.setup_memory_required = f->setup_memory_required; d.setup_temp_memory_required = f->setup_temp_memory_required; d.temp_memory_required = f->temp_memory_required; d.max_frame_size = f->blocksize_1 >> 1; return d; }
vc_comment vc_get_comment(vc_stream *f) { vc_comment d; d.vendor = f->vendor; d.comment_list_length = f->comment_list_length; d.comment_list = f->comment_list; return d; }
int vc_get_error(vc_stream *f) { int e = f->error; f->error = VC_OK; return e; }
static vc_stream * _vc_f367fd96(vc_stream *f) { vc_stream *p = (vc_stream *) setup_malloc(f, sizeof(*p)); return p; }
#ifndef VORBISCOMPILER_NO_PUSH
void vc_flush_pushdata(vc_stream *f) { f->previous_length = 0; f->page_crc_tests = 0; f->discard_samples_deferred = 0; f->current_loc_valid = FALSE; f->first_decode = FALSE; f->samples_output = 0; f->channel_buffer_start = 0; f->channel_buffer_end = 0; }
static int _vc_ac5c7d09(_Vcs *f, uint8 *data, int data_len) { int i,n; for (i=0; i < f->page_crc_tests; ++i) f->scan[i].bytes_done = 0; if (f->page_crc_tests < VORBISCOMPILER_CRC_N) { if (data_len < 4) return 0; data_len -= 3; for (i=0; i < data_len; ++i) { if (data[i] == 0x4f) { if (0==memcmp(data+i, _vc_ogg_hdr, 4)) { int j,len; uint32 crc; if (i+26 >= data_len || i+27+data[i+26] >= data_len) { data_len = i; break; } len = 27 + data[i+26]; for (j=0; j < data[i+26]; ++j) len += data[i+27+j]; crc = 0; for (j=0; j < 22; ++j) crc = _vc_a1939352(crc, data[i+j]); for ( ; j < 26; ++j) crc = _vc_a1939352(crc, 0); n = f->page_crc_tests++; f->scan[n].bytes_left = len-j; f->scan[n].crc_so_far = crc; f->scan[n].goal_crc = data[i+22] + (data[i+23] << 8) + (data[i+24]<<16) + (data[i+25]<<24); if (data[i+27+data[i+26]-1] == 255) f->scan[n].sample_loc = ~0; else f->scan[n].sample_loc = data[i+6] + (data[i+7] << 8) + (data[i+ 8]<<16) + (data[i+ 9]<<24); f->scan[n].bytes_done = i+j; if (f->page_crc_tests == VORBISCOMPILER_CRC_N) break; } } } } for (i=0; i < f->page_crc_tests;) { uint32 crc; int j; int n = f->scan[i].bytes_done; int m = f->scan[i].bytes_left; if (m > data_len - n) m = data_len - n; crc = f->scan[i].crc_so_far; for (j=0; j < m; ++j) crc = _vc_a1939352(crc, data[n+j]); f->scan[i].bytes_left -= m; f->scan[i].crc_so_far = crc; if (f->scan[i].bytes_left == 0) { if (f->scan[i].crc_so_far == f->scan[i].goal_crc) { data_len = n+m; f->page_crc_tests = -1; f->previous_length = 0; f->next_seg = -1; f->current_loc = f->scan[i].sample_loc; f->current_loc_valid = f->current_loc != ~0U; return data_len; } f->scan[i] = f->scan[--f->page_crc_tests]; } else { ++i; } } return data_len; }
int vc_decode_frame_pushdata( vc_stream *f, const uint8 *data, int data_len, int *channels, float ***output, int *samples ) { int i; int len,right,left; if (!_VC_PUSH(f)) return _vc_11f9578d(f, VC_BAD_API); if (f->page_crc_tests >= 0) { *samples = 0; return _vc_ac5c7d09(f, (uint8 *) data, data_len); } f->stream = (uint8 *) data; f->stream_end = (uint8 *) data + data_len; f->error = VC_OK; if (!_vc_7ef58f6d(f)) { *samples = 0; return 0; } if (!_vc_7256c65e(f, &len, &left, &right)) { enum vc_status error = f->error; if (error == VC_BAD_PKT) { f->error = VC_OK; while (_vc_e113cb67(f) != EOP) if (f->eof) break; *samples = 0; return (int) (f->stream - data); } if (error == VC_BAD_PKT_FLAG) { if (f->previous_length == 0) { f->error = VC_OK; while (_vc_e113cb67(f) != EOP) if (f->eof) break; *samples = 0; return (int) (f->stream - data); } } vc_flush_pushdata(f); f->error = error; *samples = 0; return 1; } len = _vc_98c006bf(f, len, left, right); for (i=0; i < f->channels; ++i) f->outputs[i] = f->channel_buffers[i] + left; if (channels) *channels = f->channels; *samples = len; *output = f->outputs; return (int) (f->stream - data); }
vc_stream *vc_open_pushdata( const unsigned char *data, int data_len, int *data_used, int *error, const vc_alloc *alloc) { vc_stream *f, p; _vc_48482100(&p, alloc); p.stream = (uint8 *) data; p.stream_end = (uint8 *) data + data_len; p.push_mode = TRUE; if (!_vc_2fc069f2(&p)) { if (p.eof) *error = VC_NEED_DATA; else *error = p.error; _vc_65a45e77(&p); return NULL; } f = _vc_f367fd96(&p); if (f) { *f = p; *data_used = (int) (f->stream - data); *error = 0; return f; } else { _vc_65a45e77(&p); return NULL; } }
#endif
unsigned int vc_get_file_offset(vc_stream *f) {
#ifndef VORBISCOMPILER_NO_PUSH
if (f->push_mode) return 0;
#endif
if (USE_MEMORY(f)) return (unsigned int) (f->stream - f->stream_start);
#ifndef VORBISCOMPILER_NO_STDIO
return (unsigned int) (ftell(f->f) - f->f_start);
#endif
}
#ifndef VORBISCOMPILER_NO_PULL
static uint32 _vc_c0a74103(vc_stream *f, uint32 *end, uint32 *last) { for(;;) { int n; if (f->eof) return 0; n = _vc_90d1fe2e(f); if (n == 0x4f) { unsigned int retry_loc = vc_get_file_offset(f); int i; if (retry_loc - 25 > f->stream_len) return 0; for (i=1; i < 4; ++i) if (_vc_90d1fe2e(f) != _vc_ogg_hdr[i]) break; if (f->eof) return 0; if (i == 4) { uint8 header[27]; uint32 i, crc, goal, len; for (i=0; i < 4; ++i) header[i] = _vc_ogg_hdr[i]; for (; i < 27; ++i) header[i] = _vc_90d1fe2e(f); if (f->eof) return 0; if (header[4] != 0) goto invalid; goal = header[22] + (header[23] << 8) + (header[24]<<16) + ((uint32)header[25]<<24); for (i=22; i < 26; ++i) header[i] = 0; crc = 0; for (i=0; i < 27; ++i) crc = _vc_a1939352(crc, header[i]); len = 0; for (i=0; i < header[26]; ++i) { int s = _vc_90d1fe2e(f); crc = _vc_a1939352(crc, s); len += s; } if (len && f->eof) return 0; for (i=0; i < len; ++i) crc = _vc_a1939352(crc, _vc_90d1fe2e(f)); if (crc == goal) { if (end) *end = vc_get_file_offset(f); if (last) { if (header[5] & 0x04) *last = 1; else *last = 0; } _vc_d6034dc6(f, retry_loc-1); return 1; } } invalid: _vc_d6034dc6(f, retry_loc); } } }
#define SAMPLE_unknown 0xffffffff
static int _vc_b25987e7(vc_stream *f, _VcPg *z) { uint8 header[27], lacing[255]; int i,len; z->page_start = vc_get_file_offset(f); _vc_fc62ee61(f, header, 27); if (header[0] != 'O' || header[1] != 'g' || header[2] != 'g' || header[3] != 'S') return 0; _vc_fc62ee61(f, lacing, header[26]); len = 0; for (i=0; i < header[26]; ++i) len += lacing[i]; z->page_end = z->page_start + 27 + header[26] + len; z->last_decoded_sample = header[6] + (header[7] << 8) + (header[8] << 16) + (header[9] << 24); _vc_d6034dc6(f, z->page_start); return 1; }
static int _vc_ab8453b6(vc_stream *f, unsigned int limit_offset) { unsigned int previous_safe, end; if (limit_offset >= 65536 && limit_offset-65536 >= f->first_audio_page_offset) previous_safe = limit_offset - 65536; else previous_safe = f->first_audio_page_offset; _vc_d6034dc6(f, previous_safe); while (_vc_c0a74103(f, &end, NULL)) { if (end >= limit_offset && vc_get_file_offset(f) < limit_offset) return 1; _vc_d6034dc6(f, end); } return 0; }
static int _vc_3f9bcb51(vc_stream *f, uint32 sample_number) { _VcPg left, right, mid; int i, start_seg_with_known_loc, end_pos, page_start; uint32 delta, stream_length, padding, last_sample_limit; double offset = 0.0, bytes_per_sample = 0.0; int probe = 0; stream_length = vc_stream_length_samples(f); if (stream_length == 0) return _vc_11f9578d(f, VC_SEEK_NO_LEN); if (sample_number > stream_length) return _vc_11f9578d(f, VC_SEEK_BAD); padding = ((f->blocksize_1 - f->blocksize_0) >> 2); if (sample_number < padding) last_sample_limit = 0; else last_sample_limit = sample_number - padding; left = f->p_first; while (left.last_decoded_sample == ~0U) { _vc_d6034dc6(f, left.page_end); if (!_vc_b25987e7(f, &left)) goto error; } right = f->p_last; assert(right.last_decoded_sample != ~0U); if (last_sample_limit <= left.last_decoded_sample) { if (vc_seek_start(f)) { if (f->current_loc > sample_number) return _vc_11f9578d(f, VC_SEEK_FAIL); return 1; } return 0; } while (left.page_end != right.page_start) { assert(left.page_end < right.page_start); delta = right.page_start - left.page_end; if (delta <= 65536) { _vc_d6034dc6(f, left.page_end); } else { if (probe < 2) { if (probe == 0) { double data_bytes = right.page_end - left.page_start; bytes_per_sample = data_bytes / right.last_decoded_sample; offset = left.page_start + bytes_per_sample * (last_sample_limit - left.last_decoded_sample); } else { double error = ((double) last_sample_limit - mid.last_decoded_sample) * bytes_per_sample; if (error >= 0 && error < 8000) error = 8000; if (error < 0 && error > -8000) error = -8000; offset += error * 2; } if (offset < left.page_end) offset = left.page_end; if (offset > right.page_start - 65536) offset = right.page_start - 65536; _vc_d6034dc6(f, (unsigned int) offset); } else { _vc_d6034dc6(f, left.page_end + (delta / 2) - 32768); } if (!_vc_c0a74103(f, NULL, NULL)) goto error; } for (;;) { if (!_vc_b25987e7(f, &mid)) goto error; if (mid.last_decoded_sample != ~0U) break; _vc_d6034dc6(f, mid.page_end); assert(mid.page_start < right.page_start); } if (mid.page_start == right.page_start) { if (probe >= 2 || delta <= 65536) break; } else { if (last_sample_limit < mid.last_decoded_sample) right = mid; else left = mid; } ++probe; } page_start = left.page_start; _vc_d6034dc6(f, page_start); if (!_vc_e7208f0c(f)) return _vc_11f9578d(f, VC_SEEK_FAIL); end_pos = f->end_seg_with_known_loc; assert(end_pos >= 0); for (;;) { for (i = end_pos; i > 0; --i) if (f->segments[i-1] != 255) break; start_seg_with_known_loc = i; if (start_seg_with_known_loc > 0 || !(f->page_flag & _VC_PF_CONT)) break; if (!_vc_ab8453b6(f, page_start)) goto error; page_start = vc_get_file_offset(f); if (!_vc_e7208f0c(f)) goto error; end_pos = f->segment_count - 1; } f->current_loc_valid = FALSE; f->last_seg = FALSE; f->valid_bits = 0; f->packet_bytes = 0; f->bytes_in_seg = 0; f->previous_length = 0; f->next_seg = start_seg_with_known_loc; for (i = 0; i < start_seg_with_known_loc; i++) _vc_c7e16815(f, f->segments[i]); if (!_vc_2a5f6090(f)) return 0; if (f->current_loc > sample_number) return _vc_11f9578d(f, VC_SEEK_FAIL); return 1; error: vc_seek_start(f); return _vc_11f9578d(f, VC_SEEK_FAIL); }
static int _vc_c0ddc7d4(_Vcs *f, int *p_left_start, int *p_left_end, int *p_right_start, int *p_right_end, int *mode) { int bits_read, bytes_read; if (!_vc_2856d2f3(f, p_left_start, p_left_end, p_right_start, p_right_end, mode)) return 0; bits_read = 1 + _vc_98fda42c(f->mode_count-1); if (f->mode_config[*mode].blockflag) bits_read += 2; bytes_read = (bits_read + 7) / 8; f->bytes_in_seg += bytes_read; f->packet_bytes -= bytes_read; _vc_c7e16815(f, -bytes_read); if (f->next_seg == -1) f->next_seg = f->segment_count - 1; else f->next_seg--; f->valid_bits = 0; return 1; }
int vc_seek_frame(vc_stream *f, unsigned int sample_number) { uint32 max_frame_samples; if (_VC_PUSH(f)) return _vc_11f9578d(f, VC_BAD_API); if (!_vc_3f9bcb51(f, sample_number)) return 0; assert(f->current_loc_valid); assert(f->current_loc <= sample_number); max_frame_samples = (f->blocksize_1*3 - f->blocksize_0) >> 2; while (f->current_loc < sample_number) { int left_start, left_end, right_start, right_end, mode, frame_samples; if (!_vc_c0ddc7d4(f, &left_start, &left_end, &right_start, &right_end, &mode)) return _vc_11f9578d(f, VC_SEEK_FAIL); frame_samples = right_start - left_start; if (f->current_loc + frame_samples > sample_number) { return 1; } else if (f->current_loc + frame_samples + max_frame_samples > sample_number) { _vc_2a5f6090(f); } else { f->current_loc += frame_samples; f->previous_length = 0; _vc_dace8834(f); _vc_b2ff18b0(f); } } if (f->current_loc != sample_number) return _vc_11f9578d(f, VC_SEEK_FAIL); return 1; }
int vc_seek(vc_stream *f, unsigned int sample_number) { if (!vc_seek_frame(f, sample_number)) return 0; if (sample_number != f->current_loc) { int n; uint32 frame_start = f->current_loc; vc_get_frame_float(f, &n, NULL); assert(sample_number > frame_start); assert(f->channel_buffer_start + (int) (sample_number-frame_start) <= f->channel_buffer_end); f->channel_buffer_start += (sample_number - frame_start); } return 1; }
int vc_seek_start(vc_stream *f) { if (_VC_PUSH(f)) { return _vc_11f9578d(f, VC_BAD_API); } _vc_d6034dc6(f, f->first_audio_page_offset); f->previous_length = 0; f->first_decode = TRUE; f->next_seg = -1; return _vc_2a5f6090(f); }
unsigned int vc_stream_length_samples(vc_stream *f) { unsigned int restore_offset, previous_safe; unsigned int end, last_page_loc; if (_VC_PUSH(f)) return _vc_11f9578d(f, VC_BAD_API); if (!f->total_samples) { unsigned int last; uint32 lo,hi; char header[6]; restore_offset = vc_get_file_offset(f); if (f->stream_len >= 65536 && f->stream_len-65536 >= f->first_audio_page_offset) previous_safe = f->stream_len - 65536; else previous_safe = f->first_audio_page_offset; _vc_d6034dc6(f, previous_safe); if (!_vc_c0a74103(f, &end, &last)) { f->error = VC_NO_LAST_PAGE; f->total_samples = 0xffffffff; goto done; } last_page_loc = vc_get_file_offset(f); while (!last) { _vc_d6034dc6(f, end); if (!_vc_c0a74103(f, &end, &last)) { break; } last_page_loc = vc_get_file_offset(f); } _vc_d6034dc6(f, last_page_loc); _vc_fc62ee61(f, (unsigned char *)header, 6); lo = _vc_a8816410(f); hi = _vc_a8816410(f); if (lo == 0xffffffff && hi == 0xffffffff) { f->error = VC_NO_LAST_PAGE; f->total_samples = SAMPLE_unknown; goto done; } if (hi) lo = 0xfffffffe; f->total_samples = lo; f->p_last.page_start = last_page_loc; f->p_last.page_end = end; f->p_last.last_decoded_sample = lo; done: _vc_d6034dc6(f, restore_offset); } return f->total_samples == SAMPLE_unknown ? 0 : f->total_samples; }
float vc_stream_length_seconds(vc_stream *f) { return vc_stream_length_samples(f) / (float) f->sample_rate; }
int vc_get_frame_float(vc_stream *f, int *channels, float ***output) { int len, right,left,i; if (_VC_PUSH(f)) return _vc_11f9578d(f, VC_BAD_API); if (!_vc_7256c65e(f, &len, &left, &right)) { f->channel_buffer_start = f->channel_buffer_end = 0; return 0; } len = _vc_98c006bf(f, len, left, right); for (i=0; i < f->channels; ++i) f->outputs[i] = f->channel_buffers[i] + left; f->channel_buffer_start = left; f->channel_buffer_end = left+len; if (channels) *channels = f->channels; if (output) *output = f->outputs; return len; }
#ifndef VORBISCOMPILER_NO_STDIO
vc_stream * vc_open_file_section(FILE *file, int close_on_free, int *error, const vc_alloc *alloc, unsigned int length) { vc_stream *f, p; _vc_48482100(&p, alloc); p.f = file; p.f_start = (uint32) ftell(file); p.stream_len = length; p.close_on_free = close_on_free; if (_vc_2fc069f2(&p)) { f = _vc_f367fd96(&p); if (f) { *f = p; _vc_2a5f6090(f); return f; } } if (error) *error = p.error; _vc_65a45e77(&p); return NULL; }
vc_stream * vc_open_file(FILE *file, int close_on_free, int *error, const vc_alloc *alloc) { unsigned int len, start; start = (unsigned int) ftell(file); fseek(file, 0, SEEK_END); len = (unsigned int) (ftell(file) - start); fseek(file, start, SEEK_SET); return vc_open_file_section(file, close_on_free, error, alloc, len); }
vc_stream * vc_open_filename(const char *filename, int *error, const vc_alloc *alloc) { FILE *f;
#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
if (0 != fopen_s(&f, filename, "rb")) f = NULL;
#else
f = fopen(filename, "rb");
#endif
if (f) return vc_open_file(f, TRUE, error, alloc); if (error) *error = VC_FILE_OPEN; return NULL; }
#endif
vc_stream * vc_open_memory(const unsigned char *data, int len, int *error, const vc_alloc *alloc) { vc_stream *f, p; if (!data) { if (error) *error = VC_EOF; return NULL; } _vc_48482100(&p, alloc); p.stream = (uint8 *) data; p.stream_end = (uint8 *) data + len; p.stream_start = (uint8 *) p.stream; p.stream_len = len; p.push_mode = FALSE; if (_vc_2fc069f2(&p)) { f = _vc_f367fd96(&p); if (f) { *f = p; _vc_2a5f6090(f); if (error) *error = VC_OK; return f; } } if (error) *error = p.error; _vc_65a45e77(&p); return NULL; }
#ifndef VORBISCOMPILER_NO_S16
#define _VC_PB_M 1
#define _VC_PB_L 2
#define _VC_PB_R 4
#define L (_VC_PB_L | _VC_PB_M)
#define C (_VC_PB_L | _VC_PB_R | _VC_PB_M)
#define R (_VC_PB_R | _VC_PB_M)
static int8 channel_position[7][6] = { { 0 }, { C }, { L, R }, { L, C, R }, { L, R, L, R }, { L, C, R, L, R }, { L, C, R, L, R, C }, }
;
#ifndef VORBISCOMPILER_NO_FAST_SCL
typedef union { float f; int i; }
float_conv;
typedef char _vc_f32_chk[sizeof(float)==4 && sizeof(int) == 4];
#define _VC_FASTDEF(x) float_conv x
#define MAGIC(SHIFT) (1.5f * (1 << (23-SHIFT)) + 0.5f/(1 << SHIFT))
#define ADDEND(SHIFT) (((150-SHIFT) << 23) + (1 << 22))
#define _VC_F2I(temp,x,s) (temp.f = (x) + MAGIC(s), temp.i - ADDEND(s))
#define _vc_chk_endian()
#else
#define _VC_F2I(temp,x,s) ((int) ((x) * (1 << (s))))
#define _vc_chk_endian()
#define _VC_FASTDEF(x)
#endif
static void _vc_eae73562(short *dest, float *src, int len) { int i; _vc_chk_endian(); for (i=0; i < len; ++i) { _VC_FASTDEF(temp); int v = _VC_F2I(temp, src[i],15); if ((unsigned int) (v + 32768) > 65535) v = v < 0 ? -32768 : 32767; dest[i] = v; } }
static void _vc_5fa6c578(int mask, short *output, int num_c, float **data, int d_offset, int len) {
#define _VC_BUF_SZ 32
float buffer[_VC_BUF_SZ]; int i,j,o,n = _VC_BUF_SZ; _vc_chk_endian(); for (o = 0; o < len; o += _VC_BUF_SZ) { memset(buffer, 0, sizeof(buffer)); if (o + n > len) n = len - o; for (j=0; j < num_c; ++j) { if (channel_position[num_c][j] & mask) { for (i=0; i < n; ++i) buffer[i] += data[j][d_offset+o+i]; } } for (i=0; i < n; ++i) { _VC_FASTDEF(temp); int v = _VC_F2I(temp,buffer[i],15); if ((unsigned int) (v + 32768) > 65535) v = v < 0 ? -32768 : 32767; output[o+i] = v; } }
#undef _VC_BUF_SZ
}
static void _vc_fba9fecf(short *output, int num_c, float **data, int d_offset, int len) {
#define _VC_BUF_SZ 32
float buffer[_VC_BUF_SZ]; int i,j,o,n = _VC_BUF_SZ >> 1; _vc_chk_endian(); for (o = 0; o < len; o += _VC_BUF_SZ >> 1) { int o2 = o << 1; memset(buffer, 0, sizeof(buffer)); if (o + n > len) n = len - o; for (j=0; j < num_c; ++j) { int m = channel_position[num_c][j] & (_VC_PB_L | _VC_PB_R); if (m == (_VC_PB_L | _VC_PB_R)) { for (i=0; i < n; ++i) { buffer[i*2+0] += data[j][d_offset+o+i]; buffer[i*2+1] += data[j][d_offset+o+i]; } } else if (m == _VC_PB_L) { for (i=0; i < n; ++i) { buffer[i*2+0] += data[j][d_offset+o+i]; } } else if (m == _VC_PB_R) { for (i=0; i < n; ++i) { buffer[i*2+1] += data[j][d_offset+o+i]; } } } for (i=0; i < (n<<1); ++i) { _VC_FASTDEF(temp); int v = _VC_F2I(temp,buffer[i],15); if ((unsigned int) (v + 32768) > 65535) v = v < 0 ? -32768 : 32767; output[o2+i] = v; } }
#undef _VC_BUF_SZ
}
static void _vc_31aa7026(int buf_c, short **buffer, int b_offset, int data_c, float **data, int d_offset, int samples) { int i; if (buf_c != data_c && buf_c <= 2 && data_c <= 6) { static int channel_selector[3][2] = { {0}, {_VC_PB_M}, {_VC_PB_L, _VC_PB_R} }; for (i=0; i < buf_c; ++i) _vc_5fa6c578(channel_selector[buf_c][i], buffer[i]+b_offset, data_c, data, d_offset, samples); } else { int limit = buf_c < data_c ? buf_c : data_c; for (i=0; i < limit; ++i) _vc_eae73562(buffer[i]+b_offset, data[i]+d_offset, samples); for ( ; i < buf_c; ++i) memset(buffer[i]+b_offset, 0, sizeof(short) * samples); } }
int vc_get_frame_s16(vc_stream *f, int num_c, short **buffer, int num_samples) { float **output = NULL; int len = vc_get_frame_float(f, NULL, &output); if (len > num_samples) len = num_samples; if (len) _vc_31aa7026(num_c, buffer, 0, f->channels, output, 0, len); return len; }
static void _vc_15bc4a5a(int buf_c, short *buffer, int data_c, float **data, int d_offset, int len) { int i; _vc_chk_endian(); if (buf_c != data_c && buf_c <= 2 && data_c <= 6) { assert(buf_c == 2); for (i=0; i < buf_c; ++i) _vc_fba9fecf(buffer, data_c, data, d_offset, len); } else { int limit = buf_c < data_c ? buf_c : data_c; int j; for (j=0; j < len; ++j) { for (i=0; i < limit; ++i) { _VC_FASTDEF(temp); float f = data[i][d_offset+j]; int v = _VC_F2I(temp, f,15); if ((unsigned int) (v + 32768) > 65535) v = v < 0 ? -32768 : 32767; *buffer++ = v; } for ( ; i < buf_c; ++i) *buffer++ = 0; } } }
int vc_get_frame_s16_interleaved(vc_stream *f, int num_c, short *buffer, int num_shorts) { float **output; int len; if (num_c == 1) return vc_get_frame_s16(f,num_c,&buffer, num_shorts); len = vc_get_frame_float(f, NULL, &output); if (len) { if (len*num_c > num_shorts) len = num_shorts / num_c; _vc_15bc4a5a(num_c, buffer, f->channels, output, 0, len); } return len; }
int vc_read_samples_s16_interleaved(vc_stream *f, int channels, short *buffer, int num_shorts) { float **outputs; int len = num_shorts / channels; int n=0; while (n < len) { int k = f->channel_buffer_end - f->channel_buffer_start; if (n+k >= len) k = len - n; if (k) _vc_15bc4a5a(channels, buffer, f->channels, f->channel_buffers, f->channel_buffer_start, k); buffer += k*channels; n += k; f->channel_buffer_start += k; if (n == len) break; if (!vc_get_frame_float(f, NULL, &outputs)) break; } return n; }
int vc_read_samples_s16(vc_stream *f, int channels, short **buffer, int len) { float **outputs; int n=0; while (n < len) { int k = f->channel_buffer_end - f->channel_buffer_start; if (n+k >= len) k = len - n; if (k) _vc_31aa7026(channels, buffer, n, f->channels, f->channel_buffers, f->channel_buffer_start, k); n += k; f->channel_buffer_start += k; if (n == len) break; if (!vc_get_frame_float(f, NULL, &outputs)) break; } return n; }
#ifndef VORBISCOMPILER_NO_STDIO
int vc_decode_filename(const char *filename, int *channels, int *sample_rate, short **output) { int data_len, offset, total, limit, error; short *data; vc_stream *v = vc_open_filename(filename, &error, NULL); if (v == NULL) return -1; limit = v->channels * 4096; *channels = v->channels; if (sample_rate) *sample_rate = v->sample_rate; offset = data_len = 0; total = limit; data = (short *) malloc(total * sizeof(*data)); if (data == NULL) { vc_close(v); return -2; } for (;;) { int n = vc_get_frame_s16_interleaved(v, v->channels, data+offset, total-offset); if (n == 0) break; data_len += n; offset += n * v->channels; if (offset + limit > total) { short *data2; total *= 2; data2 = (short *) realloc(data, total * sizeof(*data)); if (data2 == NULL) { free(data); vc_close(v); return -2; } data = data2; } } *output = data; vc_close(v); return data_len; }
#endif
int vc_decode_memory(const uint8 *mem, int len, int *channels, int *sample_rate, short **output) { int data_len, offset, total, limit, error; short *data; vc_stream *v = vc_open_memory(mem, len, &error, NULL); if (v == NULL) return -1; limit = v->channels * 4096; *channels = v->channels; if (sample_rate) *sample_rate = v->sample_rate; offset = data_len = 0; total = limit; data = (short *) malloc(total * sizeof(*data)); if (data == NULL) { vc_close(v); return -2; } for (;;) { int n = vc_get_frame_s16_interleaved(v, v->channels, data+offset, total-offset); if (n == 0) break; data_len += n; offset += n * v->channels; if (offset + limit > total) { short *data2; total *= 2; data2 = (short *) realloc(data, total * sizeof(*data)); if (data2 == NULL) { free(data); vc_close(v); return -2; } data = data2; } } *output = data; vc_close(v); return data_len; }
#endif
int vc_read_samples_f32_interleaved(vc_stream *f, int channels, float *buffer, int num_floats) { float **outputs; int len = num_floats / channels; int n=0; int z = f->channels; if (z > channels) z = channels; while (n < len) { int i,j; int k = f->channel_buffer_end - f->channel_buffer_start; if (n+k >= len) k = len - n; for (j=0; j < k; ++j) { for (i=0; i < z; ++i) *buffer++ = f->channel_buffers[i][f->channel_buffer_start+j]; for ( ; i < channels; ++i) *buffer++ = 0; } n += k; f->channel_buffer_start += k; if (n == len) break; if (!vc_get_frame_float(f, NULL, &outputs)) break; } return n; }
int vc_read_samples_f32(vc_stream *f, int channels, float **buffer, int num_samples) { float **outputs; int n=0; int z = f->channels; if (z > channels) z = channels; while (n < num_samples) { int i; int k = f->channel_buffer_end - f->channel_buffer_start; if (n+k >= num_samples) k = num_samples - n; if (k) { for (i=0; i < z; ++i) memcpy(buffer[i]+n, f->channel_buffers[i]+f->channel_buffer_start, sizeof(float)*k); for ( ; i < channels; ++i) memset(buffer[i]+n, 0, sizeof(float) * k); } n += k; f->channel_buffer_start += k; if (n == num_samples) break; if (!vc_get_frame_float(f, NULL, &outputs)) break; } return n; }
#endif
#endif

/* ============================================================
   caudio engine implementation
   ============================================================ */

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "net/minecraft/client/platform/audio/engine/caudio.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objbase.h>
#define INITGUID
#include <initguid.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#elif defined(__APPLE__)
#include <AudioToolbox/AudioToolbox.h>
#include <CoreAudio/CoreAudioTypes.h>
#include <pthread.h>
#include <unistd.h>
#elif defined(__linux__)
#include <alsa/asoundlib.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#else
#error "Unsupported platform for cleanroom caudio subset."
#endif

typedef struct {
    volatile int running;
    ca_engine* engine;
    float* mix_buf;
    ca_uint32 mix_cap;
    ca_uint32 period_frames;
#if defined(_WIN32)
    IAudioClient* client;
    IAudioRenderClient* render;
    HANDLE event;
    HANDLE thread;
#elif defined(__APPLE__)
    AudioQueueRef queue;
    pthread_t thread;
    ca_uint32 buffer_count;
    AudioQueueBufferRef* buffers;
#elif defined(__linux__)
    snd_pcm_t* pcm;
    pthread_t thread;
#endif
} ca_device;

typedef struct { vc_stream* vorbis; } ca_vorbis_backend;

typedef struct {
    const unsigned char* data;
    size_t size;
    size_t pos;
    ca_uint32 channels;
    ca_uint32 sampleRate;
    ca_uint64 frames;
} ca_wav_backend;

static ca_result ca_vorbis_read(ca_data_source* src, void* out, ca_uint64 frames, ca_uint64* read)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_vorbis_backend* vb = (ca_vorbis_backend*)dec->backend;
    if (!vb || !vb->vorbis) return CAUDIO_INVALID_ARGS;
    int ch = (int)dec->channels;
    int got = vc_read_samples_f32_interleaved(vb->vorbis, ch, (float*)out, (int)frames * ch);
    if (read) *read = (ca_uint64)(got > 0 ? got : 0);
    dec->cursor += (ca_uint64)(got > 0 ? got : 0);
    return got > 0 ? CAUDIO_OK : CAUDIO_ERROR;
}

static ca_result ca_vorbis_seek(ca_data_source* src, ca_uint64 frame)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_vorbis_backend* vb = (ca_vorbis_backend*)dec->backend;
    if (!vb || !vb->vorbis) return CAUDIO_INVALID_ARGS;
    if (!vc_seek(vb->vorbis, (unsigned int)frame)) return CAUDIO_ERROR;
    dec->cursor = frame;
    return CAUDIO_OK;
}

static ca_result ca_vorbis_format(ca_data_source* src, ca_format* fmt, ca_uint32* ch, ca_uint32* rate)
{
    ca_decoder* dec = (ca_decoder*)src;
    if (fmt) *fmt = dec->format;
    if (ch) *ch = dec->channels;
    if (rate) *rate = dec->sampleRate;
    return CAUDIO_OK;
}

static const ca_data_source_vtable g_vorbis_vtable = { ca_vorbis_read, ca_vorbis_seek, ca_vorbis_format };

static ca_result ca_wav_read(ca_data_source* src, void* out, ca_uint64 frames, ca_uint64* read)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_wav_backend* wb = (ca_wav_backend*)dec->backend;
    if (!wb) return CAUDIO_INVALID_ARGS;
    ca_uint64 avail = wb->frames - dec->cursor;
    ca_uint64 n = frames < avail ? frames : avail;
    size_t bps = (size_t)wb->channels * sizeof(float);
    memcpy(out, wb->data + (size_t)dec->cursor * bps, (size_t)n * bps);
    dec->cursor += n;
    if (read) *read = n;
    return n > 0 ? CAUDIO_OK : CAUDIO_ERROR;
}

static ca_result ca_wav_seek(ca_data_source* src, ca_uint64 frame)
{
    ca_decoder* dec = (ca_decoder*)src;
    ca_wav_backend* wb = (ca_wav_backend*)dec->backend;
    if (!wb || frame > wb->frames) return CAUDIO_INVALID_ARGS;
    dec->cursor = frame;
    return CAUDIO_OK;
}

static ca_result ca_wav_format(ca_data_source* src, ca_format* fmt, ca_uint32* ch, ca_uint32* rate)
{
    ca_decoder* dec = (ca_decoder*)src;
    if (fmt) *fmt = dec->format;
    if (ch) *ch = dec->channels;
    if (rate) *rate = dec->sampleRate;
    return CAUDIO_OK;
}

static const ca_data_source_vtable g_wav_vtable = { ca_wav_read, ca_wav_seek, ca_wav_format };

static int ca_is_ogg(const unsigned char* d, size_t n)
{
    return n >= 4 && d[0]=='O' && d[1]=='g' && d[2]=='g' && d[3]=='S';
}

static int ca_is_wav(const unsigned char* d, size_t n)
{
    return n >= 12 && d[0]=='R' && d[1]=='I' && d[2]=='F' && d[3]=='F' &&
           d[8]=='W' && d[9]=='A' && d[10]=='V' && d[11]=='E';
}

static ca_result ca_load_wav_memory(const void* data, size_t size, ca_wav_backend* wb)
{
    const unsigned char* p = (const unsigned char*)data;
    if (!ca_is_wav(p, size)) return CAUDIO_INVALID_FILE;
    ca_uint32 channels = (ca_uint32)(p[22] | (p[23] << 8));
    ca_uint32 rate = (ca_uint32)(p[24] | (p[25]<<8) | (p[26]<<16) | (p[27]<<24));
    ca_uint16 bits = (ca_uint16)(p[34] | (p[35] << 8));
    size_t pos = 36;
    while (pos + 8 <= size) {
        ca_uint32 chunk = (ca_uint32)(p[pos] | (p[pos+1]<<8) | (p[pos+2]<<16) | (p[pos+3]<<24));
        ca_uint32 csz = (ca_uint32)(p[pos+4] | (p[pos+5]<<8) | (p[pos+6]<<16) | (p[pos+7]<<24));
        pos += 8;
        if (chunk == 0x61746164u) {
            ca_uint64 frames = csz / ((ca_uint64)channels * (bits/8));
            float* pcm = (float*)malloc((size_t)frames * channels * sizeof(float));
            if (!pcm) return CAUDIO_OUT_OF_MEMORY;
            const unsigned char* src = p + pos;
            for (ca_uint64 i = 0; i < frames * channels; ++i) {
                if (bits == 16) {
                    int idx = (int)(i * 2);
                    int16_t s = (int16_t)(src[idx] | (src[idx+1] << 8));
                    pcm[i] = (float)s / 32768.0f;
                } else if (bits == 8) {
                    pcm[i] = ((float)src[i] - 128.0f) / 128.0f;
                } else {
                    free(pcm);
                    return CAUDIO_INVALID_FILE;
                }
            }
            wb->data = (const unsigned char*)pcm;
            wb->size = (size_t)frames * channels * sizeof(float);
            wb->pos = 0;
            wb->channels = channels;
            wb->sampleRate = rate;
            wb->frames = frames;
            return CAUDIO_OK;
        }
        pos += csz;
    }
    return CAUDIO_INVALID_FILE;
}

caudio_config caudio_config_init(void)
{
    caudio_config c;
    memset(&c, 0, sizeof(c));
    c.listenerCount = 1;
    return c;
}

static float ca_spatial_gain(const ca_engine* e, const ca_voice* s)
{
    if (!s->spatial) return 1.0f;
    float dx = s->pos_x - e->listener_x;
    float dy = s->pos_y - e->listener_y;
    float dz = s->pos_z - e->listener_z;
    float dist = sqrtf(dx*dx + dy*dy + dz*dz);
    if (dist <= s->min_dist) return 1.0f;
    if (dist >= s->max_dist) return 0.0f;
    float t = (dist - s->min_dist) / (s->max_dist - s->min_dist);
    if (s->rolloff <= 1.0f) return 1.0f - t;
    return 1.0f / (1.0f + s->rolloff * t * 4.0f);
}

static double ca_pitch_step(const ca_engine* e, const ca_voice* s)
{
    if (!e || !s) return 1.0;
    const ca_uint32 sourceRate = s->sampleRate != 0 ? s->sampleRate : e->sampleRate;
    if (sourceRate == 0 || e->sampleRate == 0) return (double)s->pitch;
    return (double)s->pitch * (double)sourceRate / (double)e->sampleRate;
}

static float ca_lerp_sample(const float* pcm, ca_uint64 frames, ca_uint32 channels,
    double pos, ca_uint32 out_ch)
{
    if (!pcm || frames == 0 || pos < 0.0) return 0.0f;
    ca_uint64 max_frame = frames - 1;
    if (pos >= (double)frames) {
        ca_uint32 sc = out_ch < channels ? out_ch : 0;
        return pcm[max_frame * channels + sc];
    }
    ca_uint64 i0 = (ca_uint64)pos;
    ca_uint64 i1 = i0 + 1 < frames ? i0 + 1 : i0;
    float frac = (float)(pos - (double)i0);
    ca_uint32 sc = out_ch < channels ? out_ch : 0;
    float a = pcm[i0 * channels + sc];
    float b = pcm[i1 * channels + sc];
    return a + (b - a) * frac;
}

static float ca_ring_sample(const ca_voice* s, double pos, ca_uint32 out_ch)
{
    if (!s->stream_ring || s->stream_ring_frames == 0 || s->stream_ring_fill == 0) return 0.0f;
    if (pos < 0.0) return 0.0f;
    ca_uint32 cap = s->stream_ring_frames;
    ca_uint32 ch = s->channels;
    ca_uint32 avail = s->stream_ring_fill;
    if (pos >= (double)avail) {
        ca_uint32 sc = out_ch < ch ? out_ch : 0;
        ca_uint32 last = (s->stream_ring_read + avail - 1) % cap;
        return s->stream_ring[last * ch + sc];
    }
    ca_uint32 i0 = (ca_uint32)pos;
    ca_uint32 i1 = i0 + 1 < avail ? i0 + 1 : i0;
    float frac = (float)(pos - (double)i0);
    ca_uint32 sc = out_ch < ch ? out_ch : 0;
    ca_uint32 idx0 = (s->stream_ring_read + i0) % cap;
    ca_uint32 idx1 = (s->stream_ring_read + i1) % cap;
    float a = s->stream_ring[idx0 * ch + sc];
    float b = s->stream_ring[idx1 * ch + sc];
    return a + (b - a) * frac;
}

static void ca_stream_ring_push(ca_voice* s, const float* data, ca_uint32 frames)
{
    if (!s->stream_ring || frames == 0) return;
    ca_uint32 cap = s->stream_ring_frames;
    ca_uint32 ch = s->channels;
    for (ca_uint32 i = 0; i < frames; ++i) {
        if (s->stream_ring_fill >= cap) break;
        ca_uint32 wi = s->stream_ring_write;
        for (ca_uint32 c = 0; c < ch; ++c)
            s->stream_ring[wi * ch + c] = data[i * ch + c];
        s->stream_ring_write = (wi + 1) % cap;
        ++s->stream_ring_fill;
    }
}

static void ca_stream_ring_discard(ca_voice* s, ca_uint32 frames)
{
    if (!s->stream_ring || frames == 0) return;
    ca_uint32 drop = frames < s->stream_ring_fill ? frames : s->stream_ring_fill;
    s->stream_ring_read = (s->stream_ring_read + drop) % s->stream_ring_frames;
    s->stream_ring_fill -= drop;
}

static void ca_voice_stream_refill(ca_voice* s, ca_uint32 target_frames)
{
    if (!s->streaming || !s->source) return;
    float chunk[4096];
    ca_uint32 ch = s->channels;
    while (s->stream_ring_fill < target_frames) {
        ca_uint32 want = 1024;
        if (want * ch > 4096) want = 4096 / ch;
        ca_uint64 got = 0;
        ca_result r = s->source->base.vtable->onRead(s->source, chunk, want, &got);
        if (r != CAUDIO_OK || got == 0) {
            if (got == 0) s->at_end = 1;
            break;
        }
        ca_stream_ring_push(s, chunk, (ca_uint32)got);
    }
}

static float ca_voice_sample_pcm(const ca_voice* s, const float* pcm, ca_uint64 pcm_frames,
    double src_pos, ca_uint32 out_ch)
{
    if (s->streaming) {
        double ring_pos = src_pos - (double)s->stream_frames_consumed;
        return ca_ring_sample(s, ring_pos, out_ch);
    }
    return ca_lerp_sample(pcm, pcm_frames, s->channels, src_pos, out_ch);
}

static void ca_engine_drain_deferred(ca_engine* engine)
{
    for (ca_uint32 i = 0; i < engine->defer_count; ++i) {
        free(engine->defer_pcm[i]);
        free(engine->defer_ring[i]);
        free(engine->defer_voice[i]);
    }
    engine->defer_count = 0;
}

static void ca_engine_queue_deferred(ca_engine* engine, float* pcm, float* ring, ca_voice* voice)
{
    if (engine->defer_count >= CA_DEFER_FREE_MAX) {
        free(pcm);
        free(ring);
        free(voice);
        return;
    }
    engine->defer_pcm[engine->defer_count]   = pcm;
    engine->defer_ring[engine->defer_count]  = ring;
    engine->defer_voice[engine->defer_count] = voice;
    engine->defer_count++;
}

static void ca_engine_mix(ca_engine* engine, float* out, ca_uint32 frames)
{
    if (!engine || !out || frames == 0) return;

    ca_voice* snap_voice[CA_ENGINE_MAX_VOICES];
    float* snap_pcm[CA_ENGINE_MAX_VOICES];
    ca_uint64 snap_frames[CA_ENGINE_MAX_VOICES];
    ca_uint32 snap_count = 0;

    ca_mutex_lock(&engine->lock);
    snap_count = engine->sound_count;
    if (snap_count > CA_ENGINE_MAX_VOICES) snap_count = CA_ENGINE_MAX_VOICES;
    for (ca_uint32 i = 0; i < snap_count; ++i) {
        ca_voice* s = engine->sounds[i];
        snap_voice[i] = s;
        snap_pcm[i] = s ? s->pcm : NULL;
        snap_frames[i] = s ? s->pcm_frames : 0;
    }
    ca_mutex_unlock(&engine->lock);

    ca_uint32 ch = engine->channels;
    memset(out, 0, (size_t)frames * ch * sizeof(float));
    for (ca_uint32 si = 0; si < snap_count; ++si) {
        ca_voice* s = snap_voice[si];
        const float* pcm = snap_pcm[si];
        ca_uint64 pcm_frames = snap_frames[si];
        if (!s || !s->playing || s->at_end) continue;
        float gain = engine->master_volume * s->volume * ca_spatial_gain(engine, s);
        if (gain <= 0.0f) continue;
        if (s->streaming) {
            ca_voice_stream_refill(s, 4096);
            if (s->stream_ring_fill == 0 && s->at_end) { s->playing = 0; continue; }
        } else if (!pcm || pcm_frames == 0) {
            continue;
        }
        double step = ca_pitch_step(engine, s);
        ca_uint32 produced = 0;
        for (ca_uint32 i = 0; i < frames; ++i) {
            double src_pos = s->pitch_cursor + (double)i * step;
            if (s->streaming) {
                if (src_pos >= (double)(s->stream_frames_consumed + s->stream_ring_fill)) {
                    if (s->at_end) { s->playing = 0; break; }
                    ca_voice_stream_refill(s, (ca_uint32)src_pos - s->stream_frames_consumed + 2048);
                    if (src_pos >= (double)(s->stream_frames_consumed + s->stream_ring_fill) && s->at_end) {
                        s->playing = 0; break;
                    }
                }
            } else if (src_pos >= (double)pcm_frames) {
                if (s->flags & CA_VOICE_FLAG_LOOPING && pcm_frames > 0)
                    src_pos = fmod(src_pos, (double)pcm_frames);
                else { s->at_end = 1; s->playing = 0; break; }
            }
            for (ca_uint32 c = 0; c < ch; ++c)
                out[i * ch + c] += ca_voice_sample_pcm(s, pcm, pcm_frames, src_pos, c) * gain;
            produced = i + 1;
        }
        if (produced > 0) {
            s->pitch_cursor += (double)produced * step;
            if (s->streaming) {
                ca_uint32 new_consumed = (ca_uint32)s->pitch_cursor;
                if (new_consumed > s->stream_frames_consumed) {
                    ca_stream_ring_discard(s, new_consumed - s->stream_frames_consumed);
                    s->stream_frames_consumed = new_consumed;
                }
            }
        }
        if (!s->streaming && s->pitch_cursor >= (double)pcm_frames) {
            if (s->flags & CA_VOICE_FLAG_LOOPING && pcm_frames > 0)
                s->pitch_cursor = fmod(s->pitch_cursor, (double)pcm_frames);
            else { s->at_end = 1; s->playing = 0; }
        }
    }

    ca_mutex_lock(&engine->lock);
    ca_engine_drain_deferred(engine);
    ca_mutex_unlock(&engine->lock);
}

#if defined(_WIN32)
static DWORD WINAPI ca_device_thread(LPVOID param)
{
    ca_device* dev = (ca_device*)param;
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    while (dev->running) {
        WaitForSingleObject(dev->event, INFINITE);
        if (!dev->running) break;
        UINT32 padding = 0;
        dev->client->lpVtbl->GetCurrentPadding(dev->client, &padding);
        UINT32 buffer_frames = 0;
        dev->client->lpVtbl->GetBufferSize(dev->client, &buffer_frames);
        UINT32 need = buffer_frames - padding;
        if (need == 0) continue;
        BYTE* dst = NULL;
        dev->render->lpVtbl->GetBuffer(dev->render, need, &dst);
        ca_uint32 total = need * dev->engine->channels;
        if (dev->mix_cap < total) {
            float* nb = (float*)realloc(dev->mix_buf, (size_t)total * sizeof(float));
            if (nb) { dev->mix_buf = nb; dev->mix_cap = total; }
        }
        if (dev->mix_buf) {
            ca_engine_mix(dev->engine, dev->mix_buf, need);
            memcpy(dst, dev->mix_buf, (size_t)total * sizeof(float));
        } else {
            memset(dst, 0, (size_t)total * sizeof(float));
        }
        dev->render->lpVtbl->ReleaseBuffer(dev->render, need, 0);
    }
    CoUninitialize();
    return 0;
}
#elif defined(__APPLE__)
static void ca_audio_queue_callback(void* user, AudioQueueRef queue, AudioQueueBufferRef buf)
{
    ca_device* dev = (ca_device*)user;
    if (!dev || !dev->running || !dev->engine) {
        memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
        AudioQueueEnqueueBuffer(queue, buf, 0, NULL);
        return;
    }
    ca_uint32 frames = dev->period_frames;
    ca_uint32 total = frames * dev->engine->channels;
    if (dev->mix_cap < total) {
        float* nb = (float*)realloc(dev->mix_buf, (size_t)total * sizeof(float));
        if (nb) { dev->mix_buf = nb; dev->mix_cap = total; }
    }
    if (dev->mix_buf) {
        ca_engine_mix(dev->engine, dev->mix_buf, frames);
        memcpy(buf->mAudioData, dev->mix_buf, (size_t)total * sizeof(float));
    } else {
        memset(buf->mAudioData, 0, buf->mAudioDataByteSize);
    }
    AudioQueueEnqueueBuffer(dev->queue, buf, 0, NULL);
}

static void* ca_device_thread(void* param)
{
    ca_device* dev = (ca_device*)param;
    while (dev->running) usleep(20000);
    return NULL;
}
#elif defined(__linux__)
static void* ca_device_thread(void* param)
{
    ca_device* dev = (ca_device*)param;
    ca_uint32 frames = dev->period_frames;
    ca_uint32 total = frames * dev->engine->channels;
    if (dev->mix_cap < total) {
        dev->mix_buf = (float*)realloc(dev->mix_buf, (size_t)total * sizeof(float));
        if (dev->mix_buf) dev->mix_cap = total;
    }
    while (dev->running) {
        if (dev->mix_buf) {
            ca_engine_mix(dev->engine, dev->mix_buf, frames);
            snd_pcm_sframes_t written = snd_pcm_writei(dev->pcm, dev->mix_buf, (snd_pcm_uframes_t)frames);
            if (written == -EPIPE) snd_pcm_prepare(dev->pcm);
            else if (written < 0) snd_pcm_recover(dev->pcm, (int)written, 0);
        } else {
            usleep(20000);
        }
    }
    return NULL;
}
#endif

static ca_result ca_device_start(ca_engine* engine, ca_device** out_dev)
{
    ca_device* dev = (ca_device*)calloc(1, sizeof(*dev));
    if (!dev) return CAUDIO_OUT_OF_MEMORY;
    dev->engine = engine;
    dev->period_frames = engine->sampleRate / 50;
    if (dev->period_frames < 256) dev->period_frames = 256;
    dev->running = 1;

#if defined(_WIN32)
    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    IMMDeviceEnumerator* enumerator = NULL;
    IMMDevice* device = NULL;
    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
            &IID_IMMDeviceEnumerator, (void**)&enumerator))) { free(dev); return CAUDIO_ERROR; }
    if (FAILED(enumerator->lpVtbl->GetDefaultAudioEndpoint(enumerator, eRender, eConsole, &device))) {
        enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    if (FAILED(device->lpVtbl->Activate(device, &IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&dev->client))) {
        device->lpVtbl->Release(device); enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    WAVEFORMATEX fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
    fmt.nChannels = (WORD)engine->channels;
    fmt.nSamplesPerSec = engine->sampleRate;
    fmt.wBitsPerSample = 32;
    fmt.nBlockAlign = (WORD)(fmt.nChannels * 4);
    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
    if (FAILED(dev->client->lpVtbl->Initialize(dev->client, AUDCLNT_SHAREMODE_SHARED,
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 10000000 / 20, 0, &fmt, NULL))) {
        dev->client->lpVtbl->Release(dev->client); device->lpVtbl->Release(device);
        enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    if (FAILED(dev->client->lpVtbl->GetService(dev->client, &IID_IAudioRenderClient, (void**)&dev->render))) {
        dev->client->lpVtbl->Release(dev->client); device->lpVtbl->Release(device);
        enumerator->lpVtbl->Release(enumerator); free(dev); return CAUDIO_ERROR;
    }
    dev->event = CreateEventA(NULL, FALSE, FALSE, NULL);
    dev->client->lpVtbl->SetEventHandle(dev->client, dev->event);
    dev->thread = CreateThread(NULL, 0, ca_device_thread, dev, 0, NULL);
    dev->client->lpVtbl->Start(dev->client);
    device->lpVtbl->Release(device);
    enumerator->lpVtbl->Release(enumerator);
#elif defined(__APPLE__)
    AudioStreamBasicDescription asbd;
    memset(&asbd, 0, sizeof(asbd));
    asbd.mSampleRate = engine->sampleRate;
    asbd.mFormatID = kAudioFormatLinearPCM;
    asbd.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    asbd.mBitsPerChannel = 32;
    asbd.mChannelsPerFrame = engine->channels;
    asbd.mFramesPerPacket = 1;
    asbd.mBytesPerFrame = engine->channels * sizeof(float);
    asbd.mBytesPerPacket = asbd.mBytesPerFrame;
    if (AudioQueueNewOutput(&asbd, ca_audio_queue_callback, dev, NULL, NULL, 0, &dev->queue) != noErr) {
        free(dev); return CAUDIO_ERROR;
    }
    dev->buffer_count = 3;
    dev->buffers = (AudioQueueBufferRef*)calloc(dev->buffer_count, sizeof(AudioQueueBufferRef));
    if (!dev->buffers) { AudioQueueDispose(dev->queue, CA_TRUE); free(dev); return CAUDIO_OUT_OF_MEMORY; }
    ca_uint32 bytes = dev->period_frames * engine->channels * sizeof(float);
    for (ca_uint32 i = 0; i < dev->buffer_count; ++i) {
        if (AudioQueueAllocateBuffer(dev->queue, bytes, &dev->buffers[i]) != noErr) {
            AudioQueueDispose(dev->queue, CA_TRUE); free(dev->buffers); free(dev); return CAUDIO_ERROR;
        }
        dev->buffers[i]->mAudioDataByteSize = bytes;
        memset(dev->buffers[i]->mAudioData, 0, bytes);
        AudioQueueEnqueueBuffer(dev->queue, dev->buffers[i], 0, NULL);
    }
    if (AudioQueueStart(dev->queue, NULL) != noErr) {
        AudioQueueDispose(dev->queue, CA_TRUE); free(dev->buffers); free(dev); return CAUDIO_ERROR;
    }
    pthread_create(&dev->thread, NULL, ca_device_thread, dev);
#elif defined(__linux__)
    if (snd_pcm_open(&dev->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) { free(dev); return CAUDIO_ERROR; }
    snd_pcm_hw_params_t* hw = NULL;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(dev->pcm, hw);
    snd_pcm_hw_params_set_access(dev->pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(dev->pcm, hw, SND_PCM_FORMAT_FLOAT_LE);
    snd_pcm_hw_params_set_channels(dev->pcm, hw, engine->channels);
    unsigned int rate = engine->sampleRate;
    snd_pcm_hw_params_set_rate_near(dev->pcm, hw, &rate, NULL);
    snd_pcm_uframes_t period = dev->period_frames;
    snd_pcm_hw_params_set_period_size_near(dev->pcm, hw, &period, NULL);
    if (snd_pcm_hw_params(dev->pcm, hw) < 0) { snd_pcm_close(dev->pcm); free(dev); return CAUDIO_ERROR; }
    snd_pcm_prepare(dev->pcm);
    pthread_create(&dev->thread, NULL, ca_device_thread, dev);
#endif
    *out_dev = dev;
    return CAUDIO_OK;
}

static void ca_device_stop(ca_device* dev)
{
    if (!dev) return;
    dev->running = 0;
#if defined(_WIN32)
    if (dev->event) SetEvent(dev->event);
    if (dev->thread) { WaitForSingleObject(dev->thread, INFINITE); CloseHandle(dev->thread); }
    if (dev->client) dev->client->lpVtbl->Stop(dev->client);
    if (dev->render) dev->render->lpVtbl->Release(dev->render);
    if (dev->client) dev->client->lpVtbl->Release(dev->client);
    if (dev->event) CloseHandle(dev->event);
#elif defined(__APPLE__)
    if (dev->queue) { AudioQueueStop(dev->queue, CA_TRUE); AudioQueueDispose(dev->queue, CA_TRUE); }
    if (dev->thread) pthread_join(dev->thread, NULL);
    free(dev->buffers);
#elif defined(__linux__)
    if (dev->thread) pthread_join(dev->thread, NULL);
    if (dev->pcm) snd_pcm_close(dev->pcm);
#endif
    free(dev->mix_buf);
    free(dev);
}

ca_result caudio_engine_init_internal(const caudio_config* cfg, ca_engine* engine)
{
    if (!cfg || !engine) return CAUDIO_INVALID_ARGS;
    memset(engine, 0, sizeof(*engine));
    engine->channels = cfg->channels ? cfg->channels : 2;
    engine->sampleRate = cfg->sampleRate ? cfg->sampleRate : 44100;
    engine->listenerCount = cfg->listenerCount ? cfg->listenerCount : 1;
    engine->master_volume = 1.0f;
    engine->listener_at_z = -1.0f;
    engine->listener_up_y = 1.0f;
    ca_result mr = ca_mutex_init(&engine->lock);
    if (mr != CAUDIO_OK) return mr;
    ca_device* dev = NULL;
    ca_result r = ca_device_start(engine, &dev);
    if (r != CAUDIO_OK) {
        ca_mutex_uninit(&engine->lock);
        return r;
    }
    engine->device = dev;
    return CAUDIO_OK;
}

void caudio_engine_uninit_internal(ca_engine* engine)
{
    if (!engine) return;
    ca_device_stop((ca_device*)engine->device);
    engine->device = NULL;
    ca_mutex_lock(&engine->lock);
    ca_engine_drain_deferred(engine);
    ca_mutex_unlock(&engine->lock);
    ca_mutex_uninit(&engine->lock);
}

void ca_engine_set_volume(ca_engine* engine, float volume) { if (engine) engine->master_volume = volume; }

ca_result ca_engine_listener_set_position(ca_engine* engine, ca_uint32 idx, float x, float y, float z)
{
    (void)idx; if (!engine) return CAUDIO_INVALID_ARGS;
    engine->listener_x = x; engine->listener_y = y; engine->listener_z = z;
    return CAUDIO_OK;
}

ca_result ca_engine_listener_set_direction(ca_engine* engine, ca_uint32 idx, float x, float y, float z)
{
    (void)idx; if (!engine) return CAUDIO_INVALID_ARGS;
    engine->listener_at_x = x; engine->listener_at_y = y; engine->listener_at_z = z;
    return CAUDIO_OK;
}

ca_result ca_engine_listener_set_world_up(ca_engine* engine, ca_uint32 idx, float x, float y, float z)
{
    (void)idx; if (!engine) return CAUDIO_INVALID_ARGS;
    engine->listener_up_x = x; engine->listener_up_y = y; engine->listener_up_z = z;
    return CAUDIO_OK;
}

ca_result ca_mutex_init(ca_mutex* m)
{
    if (!m) return CAUDIO_INVALID_ARGS;
#if defined(_WIN32)
    CRITICAL_SECTION* cs = (CRITICAL_SECTION*)malloc(sizeof(CRITICAL_SECTION));
    if (!cs) return CAUDIO_OUT_OF_MEMORY;
    InitializeCriticalSection(cs);
    m->handle = cs;
#else
    pthread_mutex_t* pm = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
    if (!pm) return CAUDIO_OUT_OF_MEMORY;
    if (pthread_mutex_init(pm, NULL) != 0) { free(pm); return CAUDIO_ERROR; }
    m->handle = pm;
#endif
    return CAUDIO_OK;
}

void ca_mutex_uninit(ca_mutex* m)
{
    if (!m || !m->handle) return;
#if defined(_WIN32)
    DeleteCriticalSection((CRITICAL_SECTION*)m->handle);
#else
    pthread_mutex_destroy((pthread_mutex_t*)m->handle);
#endif
    free(m->handle);
    m->handle = NULL;
}

void ca_mutex_lock(ca_mutex* m)
{
    if (!m || !m->handle) return;
#if defined(_WIN32)
    EnterCriticalSection((CRITICAL_SECTION*)m->handle);
#else
    pthread_mutex_lock((pthread_mutex_t*)m->handle);
#endif
}

void ca_mutex_unlock(ca_mutex* m)
{
    if (!m || !m->handle) return;
#if defined(_WIN32)
    LeaveCriticalSection((CRITICAL_SECTION*)m->handle);
#else
    pthread_mutex_unlock((pthread_mutex_t*)m->handle);
#endif
}

ca_result ca_decoder_init_memory(const void* data, size_t size, const void* cfg, ca_decoder* dec)
{
    (void)cfg;
    if (!data || size == 0 || !dec) return CAUDIO_INVALID_ARGS;
    memset(dec, 0, sizeof(*dec));
    const unsigned char* p = (const unsigned char*)data;
    if (ca_is_ogg(p, size)) {
        ca_vorbis_backend* vb = (ca_vorbis_backend*)calloc(1, sizeof(*vb));
        if (!vb) return CAUDIO_OUT_OF_MEMORY;
        int err = 0;
        vb->vorbis = vc_open_memory(p, (int)size, &err, NULL);
        if (!vb->vorbis) { free(vb); return CAUDIO_INVALID_FILE; }
        vc_stream_info info = vc_get_info(vb->vorbis);
        dec->backend = vb;
        dec->backend_kind = 1;
        dec->base.vtable = &g_vorbis_vtable;
        dec->base.backend = dec;
        dec->format = ca_format_f32;
        dec->channels = (ca_uint32)info.channels;
        dec->sampleRate = info.sample_rate;
        dec->length = vc_stream_length_samples(vb->vorbis);
        return CAUDIO_OK;
    }
    if (ca_is_wav(p, size)) {
        ca_wav_backend* wb = (ca_wav_backend*)calloc(1, sizeof(*wb));
        if (!wb) return CAUDIO_OUT_OF_MEMORY;
        ca_result r = ca_load_wav_memory(data, size, wb);
        if (r != CAUDIO_OK) { free(wb); return r; }
        dec->backend = wb;
        dec->backend_kind = 2;
        dec->base.vtable = &g_wav_vtable;
        dec->base.backend = dec;
        dec->format = ca_format_f32;
        dec->channels = wb->channels;
        dec->sampleRate = wb->sampleRate;
        dec->length = wb->frames;
        return CAUDIO_OK;
    }
    return CAUDIO_INVALID_FILE;
}

ca_result ca_decoder_uninit(ca_decoder* dec)
{
    if (!dec) return CAUDIO_INVALID_ARGS;
    if (dec->backend_kind == 1) {
        ca_vorbis_backend* vb = (ca_vorbis_backend*)dec->backend;
        if (vb) { if (vb->vorbis) vc_close(vb->vorbis); free(vb); }
    } else if (dec->backend_kind == 2) {
        ca_wav_backend* wb = (ca_wav_backend*)dec->backend;
        if (wb) { free((void*)wb->data); free(wb); }
    }
    memset(dec, 0, sizeof(*dec));
    return CAUDIO_OK;
}

static ca_result ca_voice_decode_all(ca_voice* s)
{
    if (!s->source) return CAUDIO_INVALID_ARGS;
    ca_format fmt; ca_uint32 ch, rate;
    s->source->base.vtable->onFormat(s->source, &fmt, &ch, &rate);
    s->channels = ch; s->sampleRate = rate;
    ca_uint64 cap = 4096, total = 0;
    float* buf = (float*)malloc((size_t)cap * ch * sizeof(float));
    if (!buf) return CAUDIO_OUT_OF_MEMORY;
    for (;;) {
        ca_uint64 got = 0;
        ca_result r = s->source->base.vtable->onRead(s->source, buf + total * ch, cap - total, &got);
        if (r != CAUDIO_OK || got == 0) break;
        total += got;
        if (total + 4096 > cap) {
            cap *= 2;
            float* nb = (float*)realloc(buf, (size_t)cap * ch * sizeof(float));
            if (!nb) { free(buf); return CAUDIO_OUT_OF_MEMORY; }
            buf = nb;
        }
    }
    s->pcm = buf;
    s->pcm_frames = total;
    s->cursor = 0;
    s->owns_pcm = 1;
    return CAUDIO_OK;
}

static ca_result ca_voice_init_streaming(ca_voice* s)
{
    if (!s->source) return CAUDIO_INVALID_ARGS;
    ca_format fmt; ca_uint32 ch, rate;
    s->source->base.vtable->onFormat(s->source, &fmt, &ch, &rate);
    s->channels = ch;
    s->sampleRate = rate;
    s->streaming = 1;
    s->stream_ring_frames = CA_STREAM_RING_FRAMES;
    s->stream_ring = (float*)calloc((size_t)s->stream_ring_frames * ch, sizeof(float));
    if (!s->stream_ring) return CAUDIO_OUT_OF_MEMORY;
    ca_voice_stream_refill(s, 4096);
    return CAUDIO_OK;
}

ca_result ca_voice_init_from_data_source(ca_engine* engine, ca_data_source* src, ca_uint32 flags,
    void* group, ca_voice* sound)
{
    (void)group;
    if (!engine || !src || !sound) return CAUDIO_INVALID_ARGS;
    memset(sound, 0, sizeof(*sound));
    sound->engine = engine;
    sound->source = src;
    sound->flags = flags;
    sound->volume = 1.0f;
    sound->pitch = 1.0f;
    sound->min_dist = 1.0f;
    sound->max_dist = 16.0f;
    sound->rolloff = 1.5f;
    sound->spatial = !(flags & CA_VOICE_FLAG_NO_SPATIALIZATION);
    if (flags & CA_VOICE_FLAG_DECODE) {
        ca_result r = (flags & CA_VOICE_FLAG_STREAM) ? ca_voice_init_streaming(sound) : ca_voice_decode_all(sound);
        if (r != CAUDIO_OK) return r;
    }
    return CAUDIO_OK;
}

ca_result ca_voice_attach_engine(ca_engine* engine, ca_voice* sound)
{
    if (!engine || !sound) return CAUDIO_INVALID_ARGS;
    if (engine->sound_count < CA_ENGINE_MAX_VOICES)
        engine->sounds[engine->sound_count++] = sound;
    return CAUDIO_OK;
}

void ca_voice_init_borrowed_pcm(ca_voice* sound, ca_engine* engine, float* pcm, ca_uint64 frames,
    ca_uint32 channels, ca_uint32 sample_rate, ca_uint32 flags)
{
    if (!sound) return;
    memset(sound, 0, sizeof(*sound));
    sound->engine = engine;
    sound->pcm = pcm;
    sound->pcm_frames = frames;
    sound->channels = channels;
    sound->sampleRate = sample_rate;
    sound->flags = flags;
    sound->volume = 1.0f;
    sound->pitch = 1.0f;
    sound->min_dist = 1.0f;
    sound->max_dist = 16.0f;
    sound->rolloff = 1.5f;
    sound->spatial = !(flags & CA_VOICE_FLAG_NO_SPATIALIZATION);
    sound->owns_pcm = 0;
}

static ca_result ca_read_file(const char* path, unsigned char** out, size_t* out_len)
{
    FILE* f = fopen(path, "rb");
    if (!f) return CAUDIO_INVALID_FILE;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return CAUDIO_INVALID_FILE; }
    unsigned char* buf = (unsigned char*)malloc((size_t)sz);
    if (!buf) { fclose(f); return CAUDIO_OUT_OF_MEMORY; }
    if (fread(buf, 1, (size_t)sz, f) != (size_t)sz) { free(buf); fclose(f); return CAUDIO_ERROR; }
    fclose(f);
    *out = buf;
    *out_len = (size_t)sz;
    return CAUDIO_OK;
}

ca_result ca_voice_init_from_file(ca_engine* engine, const char* path, ca_uint32 flags,
    void* group, void* fence, ca_voice* sound)
{
    (void)fence; (void)group;
    unsigned char* data = NULL;
    size_t len = 0;
    ca_result r = ca_read_file(path, &data, &len);
    if (r != CAUDIO_OK) return r;
    ca_decoder* dec = (ca_decoder*)calloc(1, sizeof(*dec));
    if (!dec) { free(data); return CAUDIO_OUT_OF_MEMORY; }
    r = ca_decoder_init_memory(data, len, NULL, dec);
    free(data);
    if (r != CAUDIO_OK) { free(dec); return r; }
    r = ca_voice_init_from_data_source(engine, (ca_data_source*)dec, flags, NULL, sound);
    if (r != CAUDIO_OK) { ca_decoder_uninit(dec); free(dec); return r; }
    sound->owns_source = 1;
    return CAUDIO_OK;
}

void ca_voice_uninit(ca_voice* sound)
{
    if (!sound) return;
    ca_engine* engine = sound->engine;
    float* pcm = sound->pcm;
    float* ring = sound->stream_ring;
    int owns_pcm = sound->owns_pcm;
    sound->pcm = NULL;
    sound->stream_ring = NULL;
    sound->playing = 0;

    if (sound->owns_source && sound->source) {
        ca_decoder_uninit((ca_decoder*)sound->source);
        free(sound->source);
        sound->source = NULL;
    }

    if (engine) {
        for (ca_uint32 i = 0; i < engine->sound_count; ++i) {
            if (engine->sounds[i] == sound) {
                engine->sounds[i] = engine->sounds[--engine->sound_count];
                break;
            }
        }
        if (!owns_pcm) pcm = NULL;
        ca_engine_queue_deferred(engine, pcm, ring, sound);
    } else {
        if (owns_pcm) free(pcm);
        free(ring);
        free(sound);
    }
}

ca_result ca_voice_start(ca_voice* sound)
{
    if (!sound) return CAUDIO_INVALID_ARGS;
    sound->playing = 1;
    sound->at_end = 0;
    sound->cursor = 0;
    sound->pitch_cursor = 0.0;
    if (sound->streaming) {
        sound->stream_ring_read = 0;
        sound->stream_ring_write = 0;
        sound->stream_ring_fill = 0;
        sound->stream_frames_consumed = 0;
        sound->at_end = 0;
        ca_voice_stream_refill(sound, 4096);
    }
    return CAUDIO_OK;
}

ca_result ca_voice_stop(ca_voice* sound)
{
    if (!sound) return CAUDIO_INVALID_ARGS;
    sound->playing = 0;
    return CAUDIO_OK;
}

void ca_voice_set_volume(ca_voice* sound, float volume) { if (sound) sound->volume = volume; }
void ca_voice_set_pitch(ca_voice* sound, float pitch)
{
    if (!sound) return;
    if (pitch < 0.5f) {
        pitch = 0.5f;
    } else if (pitch > 2.0f) {
        pitch = 2.0f;
    } else if (pitch <= 0.0f) {
        pitch = 1.0f;
    }
    sound->pitch = pitch;
}
void ca_voice_set_position(ca_voice* sound, float x, float y, float z)
{
    if (!sound) return;
    sound->pos_x = x; sound->pos_y = y; sound->pos_z = z;
}
void ca_voice_set_min_distance(ca_voice* sound, float d) { if (sound) sound->min_dist = d; }
void ca_voice_set_max_distance(ca_voice* sound, float d) { if (sound) sound->max_dist = d; }
void ca_voice_set_rolloff(ca_voice* sound, float r) { if (sound) sound->rolloff = r; }

ca_bool32 ca_voice_is_playing(const ca_voice* sound)
{
    return sound && sound->playing && !sound->at_end ? CA_TRUE : CA_FALSE;
}

/* ============================================================
   audio backend (slot table + public C API)
   ============================================================ */

#include "net/minecraft/client/platform/audio/backend/audio_backend.h"

#include <stdint.h>

#define MAX_SLOTS 256
#define NAME_CAP  128

#define PCM_CACHE_MAX  64
#define PCM_PATH_MAX   512

typedef struct {
    char        path[PCM_PATH_MAX];
    float*      pcm;
    ca_uint64   frames;
    ca_uint32   channels;
    ca_uint32   sample_rate;
} PcmCacheEntry;

static PcmCacheEntry s_pcm_cache[PCM_CACHE_MAX];
static int s_pcm_cache_count = 0;

static const PcmCacheEntry* pcm_cache_find(const char* path)
{
    if (!path) return NULL;
    for (int i = 0; i < s_pcm_cache_count; ++i)
        if (strcmp(s_pcm_cache[i].path, path) == 0)
            return &s_pcm_cache[i];
    return NULL;
}

static void pcm_cache_store(const char* path, float* pcm, ca_uint64 frames,
    ca_uint32 channels, ca_uint32 sample_rate)
{
    if (!path || !pcm || s_pcm_cache_count >= PCM_CACHE_MAX) return;
    PcmCacheEntry* e = &s_pcm_cache[s_pcm_cache_count++];
    strncpy(e->path, path, PCM_PATH_MAX - 1);
    e->path[PCM_PATH_MAX - 1] = '\0';
    e->pcm = pcm;
    e->frames = frames;
    e->channels = channels;
    e->sample_rate = sample_rate;
}

static void pcm_cache_clear(void)
{
    for (int i = 0; i < s_pcm_cache_count; ++i)
        free(s_pcm_cache[i].pcm);
    s_pcm_cache_count = 0;
}

typedef struct {
    char        name[NAME_CAP];
    ca_voice*   voice;
    ca_decoder* dec;
    void*       mem;
    bool        inited;
} Slot;

struct AudioBackend {
    ca_engine engine;
    bool      ready;
    Slot      slots[MAX_SLOTS];
};

static void slot_clear(Slot* s)
{
    if (s->voice) {
        if (s->inited) {
            ca_voice_stop(s->voice);
            ca_voice_uninit(s->voice);
        } else {
            free(s->voice);
        }
        s->voice = NULL;
    }
    if (s->dec) {
        ca_decoder_uninit(s->dec);
        free(s->dec);
        s->dec = NULL;
    }
    free(s->mem);
    s->mem     = NULL;
    s->inited  = false;
    s->name[0] = '\0';
}

static Slot* slot_find(AudioBackend* b, const char* name)
{
    for (int i = 0; i < MAX_SLOTS; ++i)
        if (b->slots[i].name[0] && strcmp(b->slots[i].name, name) == 0)
            return &b->slots[i];
    return NULL;
}

static Slot* slot_acquire(AudioBackend* b, const char* name)
{
    Slot* existing = slot_find(b, name);
    if (existing) { slot_clear(existing); return existing; }

    for (int i = 0; i < MAX_SLOTS; ++i)
        if (!b->slots[i].name[0])
            return &b->slots[i];

    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (strcmp(b->slots[i].name, "BgMusic") != 0 &&
            strcmp(b->slots[i].name, "streaming") != 0) {
            slot_clear(&b->slots[i]);
            return &b->slots[i];
        }
    }
    return NULL;
}

static void slot_set_name(Slot* s, const char* name)
{
    strncpy(s->name, name, NAME_CAP - 1);
    s->name[NAME_CAP - 1] = '\0';
}

static ca_uint32 voice_flags(const AudioSourceParams* p)
{
    ca_uint32 f = CA_VOICE_FLAG_DECODE;
    if (p->loop)     f |= CA_VOICE_FLAG_LOOPING;
    if (p->stream)   f |= CA_VOICE_FLAG_STREAM;
    if (!p->spatial) f |= CA_VOICE_FLAG_NO_SPATIALIZATION;
    return f;
}

static bool slot_finish(Slot* s, ca_result result, const char* what, const AudioSourceParams* p)
{
    if (result != CAUDIO_OK) {
        fprintf(stderr, "audio: failed to load '%s' (error %d)\n", what, (int)result);
        slot_clear(s);
        return false;
    }
    s->inited = true;
    if (p->spatial) {
        float max_dist = p->dist > 0.0f ? p->dist : 16.0f;
        ca_voice_set_position(s->voice, p->x, p->y, p->z);
        ca_voice_set_min_distance(s->voice, 1.0f);
        ca_voice_set_max_distance(s->voice, max_dist);
        ca_voice_set_rolloff(s->voice, p->att_model == AUDIO_ATT_LINEAR ? 1.0f : 1.5f);
    } else {
        ca_voice_set_position(s->voice, 0.0f, 0.0f, 0.0f);
    }
    return true;
}

AudioBackend* audio_backend_create(void)
{
    AudioBackend* b = (AudioBackend*)calloc(1, sizeof(AudioBackend));
    if (!b) return NULL;

    caudio_config cfg = caudio_config_init();
    cfg.channels      = 2;
    cfg.sampleRate    = 44100;
    cfg.listenerCount = 1;

    if (caudio_engine_init_internal(&cfg, &b->engine) != CAUDIO_OK) {
        free(b);
        return NULL;
    }
    b->ready = true;
    return b;
}

static void backend_stop_all_nolock(AudioBackend* b)
{
    for (int i = 0; i < MAX_SLOTS; ++i)
        if (b->slots[i].name[0])
            slot_clear(&b->slots[i]);
}

void audio_backend_destroy(AudioBackend* b)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    backend_stop_all_nolock(b);
    ca_mutex_unlock(&b->engine.lock);
    pcm_cache_clear();
    if (b->ready)
        caudio_engine_uninit_internal(&b->engine);
    free(b);
}

bool audio_backend_ready(const AudioBackend* b)
{
    return b && b->ready;
}

void audio_set_master_volume(AudioBackend* b, float v)
{
    if (!b || !b->ready) return;
    ca_mutex_lock(&b->engine.lock);
    ca_engine_set_volume(&b->engine, v);
    ca_mutex_unlock(&b->engine.lock);
}

void audio_set_listener_pos(AudioBackend* b, float x, float y, float z)
{
    if (!b || !b->ready) return;
    ca_mutex_lock(&b->engine.lock);
    ca_engine_listener_set_position(&b->engine, 0, x, y, z);
    ca_mutex_unlock(&b->engine.lock);
}

static void normalize3(float* x, float* y, float* z)
{
    float len = sqrtf(*x * *x + *y * *y + *z * *z);
    if (len > 1e-4f) { *x /= len; *y /= len; *z /= len; }
}

void audio_set_listener_dir(AudioBackend* b,
    float atX, float atY, float atZ,
    float upX, float upY, float upZ)
{
    if (!b || !b->ready) return;
    normalize3(&atX, &atY, &atZ);
    normalize3(&upX, &upY, &upZ);
    ca_mutex_lock(&b->engine.lock);
    ca_engine_listener_set_direction(&b->engine, 0, atX, atY, atZ);
    ca_engine_listener_set_world_up(&b->engine, 0, upX, upY, upZ);
    ca_mutex_unlock(&b->engine.lock);
}

bool audio_source_create_file(AudioBackend* b, const char* name,
    const char* url, AudioSourceParams params)
{
    if (!b || !b->ready || !name || !url || !url[0]) return false;

    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_acquire(b, name);
    if (!s) { ca_mutex_unlock(&b->engine.lock); return false; }
    slot_set_name(s, name);

    s->voice = (ca_voice*)calloc(1, sizeof(ca_voice));
    if (!s->voice) { slot_clear(s); ca_mutex_unlock(&b->engine.lock); return false; }
    ca_mutex_unlock(&b->engine.lock);

    const ca_uint32 flags = voice_flags(&params);
    ca_result r = CAUDIO_OK;
    const PcmCacheEntry* cached = pcm_cache_find(url);
    if (cached) {
        ca_voice_init_borrowed_pcm(s->voice, &b->engine, cached->pcm, cached->frames,
            cached->channels, cached->sample_rate, flags);
    } else {
        r = ca_voice_init_from_file(&b->engine, url, flags, NULL, NULL, s->voice);
        if (r == CAUDIO_OK && s->voice->pcm) {
            pcm_cache_store(url, s->voice->pcm, s->voice->pcm_frames,
                s->voice->channels, s->voice->sampleRate);
            s->voice->owns_pcm = 0;
        }
    }

    ca_mutex_lock(&b->engine.lock);
    if (r == CAUDIO_OK)
        r = ca_voice_attach_engine(&b->engine, s->voice);
    bool ok = slot_finish(s, r, url, &params);
    ca_mutex_unlock(&b->engine.lock);
    return ok;
}

bool audio_source_create_memory(AudioBackend* b, const char* name,
    const void* data, size_t len, AudioSourceParams params)
{
    if (!b || !b->ready || !name || !data || len == 0) return false;

    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_acquire(b, name);
    if (!s) { ca_mutex_unlock(&b->engine.lock); return false; }
    slot_set_name(s, name);

    s->mem = malloc(len);
    if (!s->mem) { slot_clear(s); ca_mutex_unlock(&b->engine.lock); return false; }
    memcpy(s->mem, data, len);

    s->dec = (ca_decoder*)calloc(1, sizeof(ca_decoder));
    if (!s->dec) { slot_clear(s); ca_mutex_unlock(&b->engine.lock); return false; }
    if (ca_decoder_init_memory(s->mem, len, NULL, s->dec) != CAUDIO_OK) {
        free(s->dec); s->dec = NULL;
        slot_clear(s);
        ca_mutex_unlock(&b->engine.lock);
        return false;
    }

    s->voice = (ca_voice*)calloc(1, sizeof(ca_voice));
    if (!s->voice) { slot_clear(s); ca_mutex_unlock(&b->engine.lock); return false; }
    ca_mutex_unlock(&b->engine.lock);

    ca_result r = ca_voice_init_from_data_source(&b->engine, (ca_data_source*)s->dec,
        voice_flags(&params), NULL, s->voice);

    ca_mutex_lock(&b->engine.lock);
    if (r == CAUDIO_OK)
        r = ca_voice_attach_engine(&b->engine, s->voice);
    bool ok = slot_finish(s, r, name, &params);
    ca_mutex_unlock(&b->engine.lock);
    return ok;
}

void audio_source_play(AudioBackend* b, const char* name)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_start(s->voice);
    ca_mutex_unlock(&b->engine.lock);
}

void audio_source_stop(AudioBackend* b, const char* name)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_stop(s->voice);
    ca_mutex_unlock(&b->engine.lock);
}

void audio_source_remove(AudioBackend* b, const char* name)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_find(b, name);
    if (s) slot_clear(s);
    ca_mutex_unlock(&b->engine.lock);
}

void audio_source_set_volume(AudioBackend* b, const char* name, float v)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_set_volume(s->voice, v);
    ca_mutex_unlock(&b->engine.lock);
}

void audio_source_set_pitch(AudioBackend* b, const char* name, float v)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_set_pitch(s->voice, v);
    ca_mutex_unlock(&b->engine.lock);
}

void audio_source_set_pos(AudioBackend* b, const char* name, float x, float y, float z)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_find(b, name);
    if (s && s->inited) ca_voice_set_position(s->voice, x, y, z);
    ca_mutex_unlock(&b->engine.lock);
}

bool audio_source_playing(AudioBackend* b, const char* name)
{
    if (!b) return false;
    ca_mutex_lock(&b->engine.lock);
    Slot* s = slot_find(b, name);
    bool playing = s && s->inited && ca_voice_is_playing(s->voice) == CA_TRUE;
    ca_mutex_unlock(&b->engine.lock);
    return playing;
}

void audio_stop_all(AudioBackend* b)
{
    if (!b) return;
    ca_mutex_lock(&b->engine.lock);
    backend_stop_all_nolock(b);
    ca_mutex_unlock(&b->engine.lock);
}
