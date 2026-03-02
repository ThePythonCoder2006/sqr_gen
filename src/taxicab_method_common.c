#include <stdio.h>
#include <inttypes.h>

#include <ncurses.h>

#define NOB_STRIP_PREFIX
#include "nob.h"

#include "types.h"
#include "taxicab_method_common.h"
#include "pow_m_sqr.h"
#include "find_sets.h"
#include "arithmetic.h"

uint8_t search_pow_m_sqr_from_taxicab_iterate_over_sets_callback(uint8_t *selected, uint32_t n, void *data)
{
  pow_m_sqr_and_da_sets_packed *pack = (pow_m_sqr_and_da_sets_packed *)data;

  if (set_has_magic_sum(selected, *(pack->M)))
  {
    rel_item *set = calloc(n, sizeof(rel_item));
    size_t k = 0;
    for (uint32_t i = 0; i < n; ++i)
      for (uint32_t j = 0; j < n; ++j)
        if (GET_AS_MAT(selected, i, j, n))
          set[k++] = i * n + j;
    printf("%zu\n", pack->rels->count);
    da_append((pack->rels), set);
  }
  return pack->rels->count < pack->requiered_sets;
}

uint8_t search_pow_m_sqr_from_taxicab_find_sets_collision_callback(uint8_t *selected, uint32_t n, void *data)
{
  pow_m_sqr_and_da_sets_packed *pack = (pow_m_sqr_and_da_sets_packed *)data;

  // find_sets_print_selection(selected, n, NULL);

  uint64_t acc = 0;
  rel_item *set = calloc(n, sizeof(rel_item));
  if (set == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  size_t k = 0;
  for (uint32_t i = 0; i < n; ++i)
    for (uint32_t j = 0; j < n; ++j)
      if (GET_AS_MAT(selected, i, j, n))
      {
        set[k++] = i * n + j;
        acc += ui_pow_ui(M_SQR_GET_AS_MAT(*pack->M, i, j), pack->M->d);
      }
#ifndef __NO_GUI__
  printw("  *");
  (void) acc;
#else
  printf("set sum = %"PRIu64"\n", acc);
#endif
  da_append((pack->rels), set);

  // printf("%u\n", pack->rels->count);
  return pack->rels->count < pack->requiered_sets;
}

