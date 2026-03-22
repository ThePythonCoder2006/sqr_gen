#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include "taxicab.h"
#include "types.h"
#include "serialize.h"
#include "pow_m_sqr.h"
#include "taxicab_method_common.h"
#include "find_latin_squares.h"
#include "probas.h"

#include "nob.h"
#include <ncurses.h>

void get_file_name_identifier(char* const buff, size_t buff_sz, const char* const prefix, const char* const suffix)
{
  time_t time_tmp = 0;
  time(&time_tmp);
  struct tm* local = localtime(&time_tmp);

  int hours, minutes, day, month, year;

  hours = local->tm_hour; // get hours since midnight (0 - 23)
  minutes = local->tm_min; // get minutes after the hour (0 - 63)
  day = local->tm_mday; // get day of month (1 - 31)
  month = local->tm_mon + 1; // get month of year (0 - 11)
  year = local->tm_year + 1900; // get the year sinc 1900

  snprintf(buff, buff_sz, "%s%02d-%02d-%d-%02d-%02d%s", prefix, day, month, year, hours, minutes, suffix);

  return;
}

void save_params(const char* const base_file_name, uint32_t r, uint32_t s)
{
  UNUSED(base_file_name);
  UNUSED(r);
  UNUSED(s);

  UNREACHABLE(__func__);
  return;
}


// -------------- taxicabs -------------------

/*
 * Format:
 * [   r   ][   s   ][   d   ][   arr   ]
 * uint32_t uint32_t uint32_t array of r*s uint32_ts
 */

void fwrite_taxicab(FILE* f, taxicab T)
{
  fwrite(&T.r, sizeof(T.r), 1, f);
  fwrite(&T.s, sizeof(T.s), 1, f);
  fwrite(&T.d, sizeof(T.d), 1, f);
  fwrite(T.arr, sizeof(*T.arr), T.r * T.s, f);
  return;
}

void save_taxicabs(const char* const base_file_name, taxicab a, const char* const a_name, taxicab b, const char* const b_name)
{
  FILE* fa = fopen(temp_sprintf("%s%s.taxicab", base_file_name, a_name), "w");
  FILE* fb = fopen(temp_sprintf("%s%s.taxicab", base_file_name, b_name), "w");

  if (fa == NULL || fb == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fwrite_taxicab(fa, a);
  fwrite_taxicab(fb, b);

  fclose(fa);
  fclose(fb);

  return;
}

void fread_taxicab(FILE* f, taxicab* T)
{
  fread(&T->r, sizeof(T->r), 1, f);
  fread(&T->s, sizeof(T->s), 1, f);
  fread(&T->d, sizeof(T->d), 1, f);
  T->arr = calloc(T->r * T->s, sizeof(*T->arr));
  if (T->arr == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
  }

  fread(T->arr, sizeof(*T->arr), T->r * T->s, f);

  return;
}

void load_taxicabs(const char* const base_file_name, taxicab* a, const char* const a_name, taxicab* b, const char* const b_name)
{
  FILE* fa = fopen(temp_sprintf("%s%s.taxicab", base_file_name, a_name), "r");
  FILE* fb = fopen(temp_sprintf("%s%s.taxicab", base_file_name, b_name), "r");

  if (fa == NULL || fb == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fread_taxicab(fa, a);
  fread_taxicab(fb, b);

  fclose(fa);
  fclose(fb);

  return;
}

// ---------------- pow_m_sqr --------------

/*
 * Format:
 * [   n   ][   d   ][        rows        ][        cols        ][         arr          ]
 * uint32_t uint32_t  array of n uint32_ts  array of n uint32_ts  array of n*n uint64_ts
 */

void fwrite_pow_m_sqr(FILE* f, pow_m_sqr M)
{
  fwrite(&M.n, sizeof(M.n), 1, f);
  fwrite(&M.d, sizeof(M.d), 1, f);
  fwrite(M.rows, sizeof(*M.arr), M.n, f);
  fwrite(M.cols, sizeof(*M.cols), M.n, f);
  fwrite(M.arr, sizeof(*M.arr), M.n * M.n, f);
  return;
}

void save_pow_m_sqr(const char* const base_file_name, pow_m_sqr M, const char* const M_name)
{
  FILE* f = fopen(temp_sprintf("%s%s.pow_m_sqr", base_file_name, M_name), "w");

  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fwrite_pow_m_sqr(f, M);

  fclose(f);
  return;
}

// ------------------ rels -----------------

/*
 * Format:
 * [   n   ][   count   ][ [   0   ][   1   ] ...items... [   count - 1   ] ]
 *  size_t     size_t     array of count arrays of n rel_items
 */

void fwrite_rels(FILE* f, da_sets rels)
{
  fwrite(&rels.n, sizeof(rels.n), 1, f);
  fwrite(&rels.count, sizeof(rels.count), 1, f);
  for (size_t i = 0; i < rels.count; ++i)
    fwrite(rels.items[i], sizeof(*rels.items[i]), rels.n, f);
  return;
}

void save_rels(const char* const base_file_name, da_sets rels, const char* const rels_name)
{
  FILE* f = fopen(temp_sprintf("%s%s.rels", base_file_name, rels_name), "w");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fwrite_rels(f, rels);

  fclose(f);
  return;
}

void fread_rels(FILE* f, da_sets* rels)
{
  fread(&rels->n, sizeof(rels->n), 1, f);
  fread(&rels->count, sizeof(rels->count), 1, f);
  for (size_t i = 0; i < rels->count; ++i)
    fread(rels->items[i], sizeof(*rels->items[i]), rels->n, f);
  return;
}

void load_rels(const char* const base_file_name, da_sets* rels, const char* const rels_name)
{
  FILE* f = fopen(temp_sprintf("%s%s.rels", base_file_name, rels_name), "r");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fread_rels(f, rels);

  fclose(f);
  return;
}


// ---------------- latin squares ---------------

/*
 * Format:
 * [   n   ][         arr         ]
 * uint32_t  array of n*n uint8_ts
 */

void fwrite_latin_square(FILE* f, latin_square P)
{
  fwrite(&P.n, sizeof(P.n), 1, f);
  fwrite(P.arr, sizeof(*P.arr), P.n * P.n, f);

  return;
}

void fread_latin_square_size(FILE* f, uint32_t* n)
{
  fread(n, sizeof(*n), 1, f);
  return;
}

void fread_latin_square_array(FILE* f, latin_square* P)
{
  fread(P->arr, sizeof(*P->arr), P->n * P->n, f);

  return;
}

void save_latin_square(const char* const base_file_name, latin_square P, const char* const P_name)
{

  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, P_name), "w");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fwrite_latin_square(f, P);

  fclose(f);

  return;
}

void load_latin_square(const char*const base_file_name, latin_square* P, const char* const P_name)
{
  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, P_name), "r");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fread_latin_square_size(f, &P->n);

  latin_square_init(P, P->n);

  fread_latin_square_array(f, P);

  fclose(f);

  return;
}

/*
 * Format:
 * [   r   ][   s   ][ [   P_0   ][   P_1   ] ...P...  [   P_{r - 1}   ] ]
 * uint32_t uint32_t  array of r latin_squares
 * [ [   Q_0   ][   Q_1   ] ...Q...  [   Q_{s - 1}   ] ]
 *  array of s latin_squares
 */

void fwrite_latin_squares(FILE* f, latin_square* P, uint32_t r, latin_square* Q, uint32_t s)
{
  fwrite(&r, sizeof(r), 1, f);
  fwrite(&s, sizeof(s), 1, f);
  for (size_t i = 0; i < r; ++i)
    fwrite_latin_square(f, P[i]);
  for (size_t j = 0; j < s; ++j)
    fwrite_latin_square(f, Q[j]);

  return;
}

// returns the sizes r, s of the latin squares arrays through the pointers
void fread_latin_squares_sizes(FILE* f, uint32_t* r, uint32_t* s)
{
  fread(r, sizeof(*r), 1, f);
  fread(s, sizeof(*s), 1, f);

  return;
}

// r, s have to be the value from fread_sizes
void read_latin_squares_arrays(FILE* f, latin_square* P, uint32_t r, latin_square* Q, uint32_t s)
{
  for (size_t i = 0; i < r; ++i)
    fread_latin_square_array(f, P + i);
  for (size_t j = 0; j < s; ++j)
    fread_latin_square_array(f, Q + j);

  return;
}

void save_latin_squares(const char*const base_file_name, latin_square* P, uint32_t r, latin_square* Q, uint32_t s, const char* const name)
{
  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "w");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fwrite_latin_squares(f, P, r, Q, s);

  fclose(f);

  return;
}

void fread_latin_squares_arrays(FILE* f, latin_square *P, uint32_t r, latin_square *Q, uint32_t s)
{
  for (size_t i = 0; i < r; ++i)
  {
    fread_latin_square_size(f, &P[i].n);
    latin_square_init(P + i, P[i].n);
    fread_latin_square_array(f, P + i);
  }
  for (size_t j = 0; j < s; ++j)
  {
    fread_latin_square_size(f, &Q[j].n);
    latin_square_init(Q + j, Q[j].n);
    fread_latin_square_array(f, Q + j);
  }

  return;
}

void load_latin_squares(const char*const base_file_name, latin_square** P, uint32_t* r, latin_square** Q, uint32_t* s, const char*const name)
{
  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "r");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fread_latin_squares_sizes(f, r, s);

  *P = calloc(*r, sizeof(**P));
  *Q = calloc(*s, sizeof(**Q));
  if (P == NULL || Q == NULL)
  {
    fprintf(stderr, "[OoM] Buy more RAM LOL!!\n");
    exit(1);
  }

  fread_latin_squares_arrays(f, *P, *r, *Q, *s);

  fclose(f);

  return;
}

/*
 * Format:
 * [   N   ][    r    ][    s    ][     [   P.arr, Q.arr   ], ...,          ]
 *  size_t   uint32_t   uint32_t    array of N arrys of latin_squares contents
 *  there r Ps each of size sxs
 *  and s Qs each of size rxr
 */

static uint8_t callback2(latin_square *_1, uint64_t _2, void *data);

static uint8_t callback1(latin_square *_1, uint64_t _2, void *data)
{
  (void)_1, (void)_2;
  iterate_over_latin_squares_array_pack *pack = (iterate_over_latin_squares_array_pack *)data;
  return iterate_over_all_square_array_callback(pack->Q, pack->s, callback2, data);
}

static uint8_t callback2(latin_square *_1, uint64_t _2, void *data)
{
  (void)_1, (void)_2;
  iterate_over_latin_squares_array_pack *pack = (iterate_over_latin_squares_array_pack *)data;

  for (uint32_t i = 0; i < pack->r; ++i)
    fwrite(pack->P[i].arr, sizeof(*pack->P[i].arr), pack->s*pack->s, pack->f);
  for (uint32_t j = 0; j < pack->s; ++j)
    fwrite(pack->Q[j].arr, sizeof(*pack->Q[j].arr), pack->r*pack->r, pack->f);

  return 1;
}

void fwrite_all_latin_square_arrays(FILE* f, latin_square* P, latin_square* Q, uint32_t r, uint32_t s)
{
  iterate_over_latin_squares_array_pack pack = {.P = P, .Q = Q, .r = r, .s = s, .f = f};

  size_t count = number_of_latin_squares(r, s);
  fwrite(&count, sizeof(count), 1, f);
  fwrite(&r, sizeof(r), 1, f);
  fwrite(&s, sizeof(s), 1, f);
  iterate_over_all_square_array_callback(P, r, callback1, &pack);

  return;
}

void save_all_latin_square_arrays(const char*const base_file_name, latin_square* P, latin_square* Q, uint32_t r, uint32_t s, const char*const name)
{
  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "w");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fwrite_all_latin_square_arrays(f, P, Q, r, s);

  fclose(f);

  return;
}

#define REFRESH_RATE (100)
/*
 * returns 1 upon early breaking
 */
uint8_t action_on_all_latin_square_arrays(const char*const base_file_name, const char*const name, perf_counter* perf, action func, void* data)
{
  FILE* f = fopen(temp_sprintf("%s%s.latin_square", base_file_name, name), "r");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  uint8_t ret = 0;
  size_t count;
  uint32_t r, s;
  fread(&count, sizeof(count), 1, f);
  fread(&r, sizeof(r), 1, f);
  fread(&s, sizeof(s), 1, f);

  latin_square *P = calloc(r, sizeof(latin_square));
  latin_square *Q = calloc(s, sizeof(latin_square));
  if (P == NULL || Q == NULL)
  {
    fprintf(stderr, "[OoM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (uint32_t i = 0; i < r; ++i)
    latin_square_init(P + i, s);
  for (uint32_t j = 0; j < s; ++j)
    latin_square_init(Q + j, r);

  for (size_t idx = 0; idx < count; ++idx)
  {
    for (uint32_t i = 0; i < r; ++i)
      fread_latin_square_array(f, P + i);
    for (uint32_t j = 0; j < s; ++j)
      fread_latin_square_array(f, Q + j);

    if (!(*func)(P, r, Q, s, data))
    {
      ret = 1;
      break;
    }

    if (idx % REFRESH_RATE == 0)
    {
#ifndef __NO_GUI__
      clear();
      printw("progress: %zu / %zu = %.2f%%\n", idx, count, ((float) idx) / count * 100.0);
      perf->counter = idx;
      perf->lcounter = idx;
      print_perfw(perf, "lsquares array");
      refresh();
#else
      printf("progress: %zu / %zu = %.2f%%\n", idx, count, ((float) idx) / count * 100.0);
      perf->counter = idx;
      perf->lcounter = idx;
      printf_perf(perf, "lsquares array");
#endif
    }
  }

  for (uint32_t i = 0; i < r; ++i)
    latin_square_clear(P + i);
  for (uint32_t j = 0; j < s; ++j)
    latin_square_clear(Q + j);

  free(P);
  free(Q);
  fclose(f);
  return ret;
}

// ------------ timings ---------------------
/*
 * Format:
 * [   peak_speed   ][   peak_lspeed   ]
 */


void fwrite_times(FILE* f, perf_counter perf)
{
  fwrite( &perf.peak_speed,  sizeof(perf.peak_speed), 1, f);
  fwrite(&perf.peak_lspeed, sizeof(perf.peak_lspeed), 1, f);

  return;
}

void save_times(const char* const base_file_name, perf_counter perf, const char* const name)
{
  FILE* f = fopen(temp_sprintf("%s%s.times", base_file_name, name), "w");
  if (f == NULL)
  {
    fprintf(stderr, "[ERROR] Could not read file: %s\n", strerror(errno));
    exit(1);
  }

  fwrite_times(f, perf);

  fclose(f);

  return;
}
