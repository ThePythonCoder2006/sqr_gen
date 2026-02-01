/*
 * size : n x n = (r*s) x (r*s)
 * block size: r x s
 *                         r blocks
 *                s cols s cols   ...   s cols
 *               |------|------|- ... -|------|
 *           r   |      |      |       |      |
 *          rows |      |      |       |      | sum = r
 *               |------|------|- ... -|------|
 *   s       .   .      .      .       .      .    .
 * blocks    .   .      .      .       .      .    .
 *           .   .      .      .       .      .    .
 *               |------|------|- ... -|------|
 *           r   |      |      |       |      | sum = r
 *          rows |      |      |       |      |
 *               |------|------|- ... -|------|
 *                sum= s sum= s   ...   sum= s
 */

#include <gmp.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "find_sets.h"
#include "pow_m_sqr.h"
#include "arithmetic.h"

#include "perf_counter.h"

#include <ncurses.h>
#include "taxicab_method.h"

// forward declaration
uint8_t find_sets_print_selection(uint8_t *selected, uint32_t n, void *_);
uint8_t iterate_over_sets_callback_inside(sets_search_data *d, uint32_t current_row, uint32_t current_col, uint32_t count, set_callback f, void *data);

int test_find_sets(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <r> <s>\n", argv[0]);
    return 1;
  }
  uint32_t r = atoi(argv[1]);
  uint32_t s = atoi(argv[2]);

  iterate_over_sets_callback(r, s, find_sets_print_selection, NULL);

  return 0;
}

uint8_t find_sets_print_selection(uint8_t *selected, uint32_t n, void *_)
{
  (void)_;

  for (uint32_t i = 0; i < n; ++i)
  {
    for (uint32_t j = 0; j < n; ++j)
      printf(GET_AS_MAT(selected, i, j, n) ? "X " : ". ");
    printf("\n");
  }
  printf("---\n");
  return 1;
}

void iterate_over_sets_callback(uint32_t r, uint32_t s, set_callback f, void *data)
{
  uint32_t n = r * s;
  sets_search_data d = {.r = r, .s = s, .n = n};

  /*
   * There are r block columns of width s
   * for all j in {0; ...; r}
   * sum_col[j] = the numbers of entries selected in the block column j
   */
  d.sum_col = calloc(r, sizeof(uint32_t));
  /*
   * There are s block rows of height r
   * for all i in {0; ...; s}
   * sum_row[i] = the numbers of entries selected in the block row i
   */
  d.sum_row = calloc(s, sizeof(uint32_t));

  d.selected = calloc(n * n, sizeof(uint8_t));

  iterate_over_sets_callback_inside(&d, 0, 0, 0, f, data);

  free(d.sum_col);
  free(d.sum_row);
  free(d.selected);

  return;
}

uint8_t iterate_over_sets_callback_inside(sets_search_data *d, uint32_t current_row, uint32_t current_col, uint32_t count, set_callback f, void *data)
{
  // base case
  if (count == d->n)
  {
    return (*f)(d->selected, d->n, data);
  }

  if (current_row == d->n)
    return 1;
  if (current_col == d->n)
    return iterate_over_sets_callback_inside(d, current_row + 1, 0, count, f, data);

  // find current block: there are s block rows and r block rows
  uint32_t block_row = current_row / d->r; // 0 <= current_row <= r*s - 1 then 0 <= block_row <= s - 1
  uint32_t block_col = current_col / d->s; // 0 <= current_col <= r*s - 1 then 0 <= block_col <= r - 1

  /*
   * 2 different cases:
   * - 1st if allowed select the current cell and continue
   * - 2nd do not select the current cell and continue
   */

  if (!GET_AS_MAT(d->selected, current_row, current_col, d->n)
      && d->sum_row[block_row] < d->r
      && d->sum_col[block_col] < d->s)
  {
    // add the current cell to the list and update counts
    GET_AS_MAT(d->selected, current_row, current_col, d->n) = 1;
    ++(d->sum_row[block_row]);
    ++(d->sum_col[block_col]);

    if (!iterate_over_sets_callback_inside(d, current_row, current_col + 1, count + 1, f, data))
      return 1;

    // roll back the changes to remove the cell
    GET_AS_MAT(d->selected, current_row, current_col, d->n) = 0;
    --(d->sum_row[block_row]);
    --(d->sum_col[block_col]);
  }

  return iterate_over_sets_callback_inside(d, current_row, current_col + 1, count, f, data);
}

// ----------- collision method ---------------

#define HSHTBL_BASE_SIZE (1024)
#define HSHTBL_FULLNESS_RATIO (0.7)

typedef struct hshtbl_node_s
{
  uint64_t sum;
  rel_item *items;
} hshtbl_node;

typedef struct
{
  size_t count, capacity;
  hshtbl_node *arr;
} hshtbl;

/*
 * hshtbl format:
 * arr is an array of capacity hshtbl_nodes
 * a node is non-empty iff its items field is non-NULL
 * collisions are resolved by finding the next (mod capacity) empty slot in arr
 */

void init_hshtbl(hshtbl* h)
{
  h->arr = calloc( HSHTBL_BASE_SIZE, sizeof(hshtbl_node));
  h->capacity = HSHTBL_BASE_SIZE;
}

#define PREFILL_CAP 1

/*
 * returns an array of n - 1 - PREFILL_CAP hshtbls. Why only n-1-PREFILL_CAP:
 *  - there is no need for a hshtbl for sets n entries long
 *  - there is no need to store partial rels of more than n-PREFILL_CAP entries, as there is no way to find any new PREFILL_CAP element long rels, checking for collisions is enough
 * hshtbl with index k will hold set of postions with k + 1 entries
 */
hshtbl *init_hshtbls(const uint32_t n)
{
  hshtbl *table = calloc(n - 1 - PREFILL_CAP, sizeof(hshtbl));
  if (table == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  for (uint32_t i = 0; i < n - 1 - PREFILL_CAP; ++i)
    init_hshtbl(table + i);
  return table;
}

void free_hshtbl(hshtbl *table)
{
  for (uint32_t i = 0; i < table->capacity; i++)
    if (table->arr[i].items != NULL)
      free(table->arr[i].items);
  free(table->arr);
  table->arr = NULL;
  return;
}

void free_hshtbls(hshtbl *table, const uint32_t n)
{
  for (uint32_t k = 0; k < n - 1 - PREFILL_CAP; ++k)
    free_hshtbl(table + k);

  free(table);

  return;
}

#if __FIND_SETS_SLIPTMIX_HASH__
static inline uint32_t hash_func(uint64_t x)
{
  // from splitmix64
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  x = x ^ (x >> 31);
  return (uint32_t)x;
}
#else
static inline uint32_t hash_func(uint64_t x)
{
  return (uint32_t)x;
}
#endif

/*
  set1 and set2 MUST have the same size of count ie set2_selected MUST have exactly count entries equal to 1 and every other being 0
 */
uint8_t sets_equal_items_selected(const rel_item *set1_items, const uint8_t *set2_selected, const uint64_t count, const uint64_t n)
{
  for (uint64_t k = 0; k < count; ++k)
  {
    const uint32_t i = set1_items[k] / n;
    const uint32_t j = set1_items[k] % n;
    if (!GET_AS_MAT(set2_selected, i, j, n))
      return 0;
  }

  return 1;
}

void hshtbl_resize(hshtbl* table)
{
  if (table->count <= table->capacity * HSHTBL_FULLNESS_RATIO)
    return;

  const size_t prev_capa = table->capacity;

  while (table->count > table->capacity * HSHTBL_FULLNESS_RATIO)
    table->capacity *= 2;

  hshtbl_node* bigger = calloc(table->capacity, sizeof(table->arr[0]));
  if (bigger == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  for (size_t i = 0; i < prev_capa; ++i)
  {
    if (table->arr[i].items != NULL)
    {
      uint32_t h = hash_func(table->arr[i].sum) % table->capacity;
      while (bigger[h].items)
        h = (h + 1) % table->capacity;

      bigger[h] = table->arr[i];
    }
  }

  free(table->arr);
  table->arr = bigger;

  return;
}

/*
 * modifies table
 * items and selected are different representations of the same data
 */
void hshtbl_insert(hshtbl *table, const rel_item *items, const uint8_t *selected, const uint32_t count, const uint64_t sum, const uint64_t n)
{
  uint64_t h = hash_func(sum) % table->capacity;

  // there should always be space in the hshtbl has its occupancy must never be above HSHTBL_FULLNESS_RATIO
  hshtbl_node node = table->arr[h];
  while (node.items)
  {
    if (sets_equal_items_selected(node.items, selected, count, n))
      return; // duplicate
    h = (h + 1) % table->capacity;
    node = table->arr[h];
  };

  node.sum = sum;
  node.items = calloc(count, sizeof(rel_item));
  if (node.items == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  memcpy(node.items, items, count * sizeof(rel_item));

  table->arr[h] = node;
  ++table->count;

  hshtbl_resize(table);

  return;
}

typedef struct
{
  uint64_t mu;
  pow_m_sqr *M;
  hshtbl found;     // pointer to single hshtbl to store already found solutions
  hshtbl *tables;    // array of hshtbls of size (n - 1)
  uint8_t *selected; // matrix of bools of size n x n
  uint32_t *row_sum, *row_sum_copy; // array of size s
  uint32_t *col_sum, *col_sum_copy; // array of size r
  rel_item *items, *items2;   // array of size n
  // lists of entry indices of open blocs/cells, there are at most r * s
  // they are both array of size r * s = s * r = n
  uint32_t *open_blocs;
  uint32_t *open_entries_in_bloc;
  perf_counter perf;
} state;

void init_state(state *pack, const uint32_t r, const uint32_t s)
{
  const size_t n = r * s;
  pack->tables = init_hshtbls(r * s);
  init_hshtbl(&pack->found);

  pack->selected = calloc(n * n, sizeof(uint8_t));
  pack->row_sum = calloc(s, sizeof(uint32_t));
  pack->col_sum = calloc(r, sizeof(uint32_t));
  pack->row_sum_copy = calloc(s, sizeof(uint32_t));
  pack->col_sum_copy = calloc(r, sizeof(uint32_t));

  pack->items = calloc(n, sizeof(rel_item));
  pack->items2 = calloc(n, sizeof(rel_item));

  // lists of entry indices of open blocs/cells, there are at most r * s
  pack->open_blocs = calloc(r * s, sizeof(uint32_t));
  pack->open_entries_in_bloc = calloc(r * s, sizeof(uint32_t));

  if (pack->selected == NULL || pack->row_sum == NULL || pack->col_sum == NULL || pack->items == NULL || pack->open_blocs == NULL || pack->open_entries_in_bloc == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  timer_start(&pack->perf.time);
  mpz_init_set_ui(pack->perf.counter, 0);
  mpf_init_set_ui(pack->perf.time_, 0);
  mpf_init_set_ui(pack->perf.speed, 0);
  mpf_init_set_ui(pack->perf.peak_speed, 0);

  return;
}

/*
 * zeroes out the field that are specific to one iteration of the algorithm
 */
void reset_state(state *pack, const uint32_t r, const uint32_t s)
{
  const size_t n = r * s;
  memset(pack->selected, 0, n * n * sizeof(*pack->selected));
  memset(pack->row_sum, 0, s * sizeof(*pack->row_sum));
  memset(pack->col_sum, 0, r * sizeof(*pack->col_sum));
  memset(pack->row_sum_copy, 0, s * sizeof(*pack->row_sum));
  memset(pack->col_sum_copy, 0, r * sizeof(*pack->col_sum));
  memset(pack->items, 0, n * sizeof(*pack->items));
  memset(pack->items2, 0, n * sizeof(*pack->items));
  memset(pack->open_blocs, 0, n * sizeof(*pack->open_blocs));
  memset(pack->open_entries_in_bloc, 0, n * sizeof(*pack->open_entries_in_bloc));
  return;
}

void free_state(state pack, const uint32_t r, const uint32_t s)
{
  free(pack.selected);
  free(pack.row_sum);
  free(pack.col_sum);
  free(pack.row_sum_copy);
  free(pack.col_sum_copy);
  free(pack.items);
  free(pack.items2);
  free(pack.open_blocs);
  free(pack.open_entries_in_bloc);

  free_hshtbls(pack.tables, r * s);
  free_hshtbl(&pack.found);

  mpz_clear(pack.perf.counter);
  mpf_clear(pack.perf.time_);
  mpf_clear(pack.perf.speed);
  (void)timer_stop(&pack.perf.time);
  return;
}

uint8_t bloc_is_filled(const uint8_t *selected, const uint32_t bi, const uint32_t bj, const uint32_t r, const uint32_t s)
{
  for (uint32_t i = 0; i < r; ++i)
    for (uint32_t j = 0; j < s; ++j)
      if (!GET_AS_MAT(selected, bi * r + i, bj * s + j, r * s))
        return 0;
  return 1;
}

/*
 * returns negative if no open blocs exist
 */
int32_t select_open_bloc(const uint8_t *selected, const uint32_t *col_sum, const uint32_t *row_sum, uint32_t *open_blocs, const uint32_t r, const uint32_t s)
{
  uint32_t open_blocs_cnt = 0;

  // iterate over all s bloc rows
  for (uint32_t bi = 0; bi < s; ++bi)
    // iterate over all r bloc columns
    for (uint32_t bj = 0; bj < r; ++bj)
      if (row_sum[bi] < r && col_sum[bj] < s && !bloc_is_filled(selected, bi, bj, r, s))
        open_blocs[open_blocs_cnt++] = bi * r + bj; // width of array is r, as there are r column blocs

  if (open_blocs_cnt == 0)
    return -1;

  // get one entry from the list we selected
  return open_blocs[rand() % open_blocs_cnt]; // don't care about proper random uniform distribution
}

uint32_t select_open_entry_in_bloc(const uint8_t *selected, uint32_t *open_entries_in_bloc,
                                   const uint32_t bi, const uint32_t bj, const uint32_t r, const uint32_t s)
{
  // iterate over all r columns in a bloc
  uint32_t open_entries_in_bloc_cnt = 0;
  for (uint32_t i = 0; i < r; ++i)
    // iterate over all s columns in a bloc
    for (uint32_t j = 0; j < s; ++j)
      if (!GET_AS_MAT(selected, bi * r + i, bj * s + j, r * s))
        open_entries_in_bloc[open_entries_in_bloc_cnt++] = (bi * r + i) * (r * s) + (bj * s + j);

  assert(open_entries_in_bloc_cnt > 0 && "Must be non-negative, as only blocs with non-zero amount of non-selected items were chosen in the previous step");

  return open_entries_in_bloc[rand() % open_entries_in_bloc_cnt];
}

int8_t are_compatible_sets(const uint8_t *selected, const rel_item *items, uint32_t *row_sum, uint32_t *col_sum, const uint32_t r, const uint32_t s, const uint32_t count)
{
  const uint32_t n = r * s;
  // check for repeated entries
  for (uint32_t idx = 0; idx < n - count; ++idx)
    if (selected[items[idx]])
      return 0;

  // check for the sum conditions
  for (uint32_t idx = 0; idx < n - count; ++idx)
  {
    uint32_t position = items[idx];
    uint32_t i = position / n;
    uint32_t j = position % n;
    uint32_t bi = i / r;
    uint32_t bj = j / s;

    ++(row_sum[bi]);
    ++(col_sum[bj]);
  }

  for (uint32_t i = 0; i < s; ++i)
    if (row_sum[i] > r)
      return 0;

  for (uint32_t j = 0; j < r; ++j)
    if (col_sum[j] > s)
      return 0;

  return 1;
}

uint64_t set_items_sqared_sum(rel_item *items, uint32_t n)
{
  uint64_t acc = 0;
  for (uint32_t i = 0; i < n; ++i)
    acc += ((uint64_t)items[i] * (uint64_t)items[i]);
  return acc;
}

uint8_t is_in_hshtbl(hshtbl h, rel_item *items, uint32_t n)
{
  uint64_t c = hash_func(set_items_sqared_sum(items, n)) % h.capacity;
  uint64_t i = c;
  hshtbl_node node = h.arr[i];
  while (node.items)
  {
    if (memcmp(items, node.items, n * sizeof(*items)) == 0)
      return 1;

    i = (i + 1) % h.capacity;
    node = h.arr[i];
  }

  return 0;
}

enum SET_SEARCH_RETVALS
{
  SIGSTOP = -1,
  NOT_FOUND,
  GUESS_FOUND,
  COLLISION_FOUND,
  RETVAL_COUNT
};

/*
 * returns:
 * |> negative value if search should stop
 * |> zero if nothing was found
 * |> positive if a collision was found
 */
int8_t check_if_set_can_be_formed_from_collision(state *pack, const uint64_t sum, const uint32_t r, const uint32_t s, const uint32_t count, set_callback f, void *data)
{
  const uint32_t n = r * s;

  // do not do collisions on lower sizes, as we do not store the sets
  if (count <= PREFILL_CAP)
    return NOT_FOUND;

  hshtbl *table = pack->tables + (n - count - 1);
  uint32_t h = hash_func(pack->mu - sum) % table->capacity;
  hshtbl_node node = table->arr[h];
  if (node.items == NULL)
    return NOT_FOUND;

  uint8_t retval = NOT_FOUND;
  // found a least match if there are no repeated entries and the union respects the sum conditions
  do
  {
    if (node.sum + sum != pack->mu)
      continue;

    // reset the row/col sum arrays
    memcpy(pack->row_sum_copy, pack->row_sum, s * sizeof(uint32_t));
    memcpy(pack->col_sum_copy, pack->col_sum, r * sizeof(uint32_t));

    if (!are_compatible_sets(pack->selected, node.items, pack->row_sum_copy, pack->col_sum_copy, r, s, count))
      continue;

    // add entries from the match
    for (uint32_t i = 0; i < n - count; ++i)
    {
      uint32_t pos = node.items[i];
      pack->selected[pos] = 1;
    }

    memset(pack->items2, 0, n * sizeof(*node.items));
    for (uint32_t k = 0, idx = 0; k < count && idx < n * n; ++idx)
    {
      if (pack->selected[idx])
        pack->items2[k++] = (rel_item)idx;
    }
    for (uint32_t k = count; k < n; ++k)
      pack->items2[k] = node.items[k - count];

    if (!is_in_hshtbl(pack->found, pack->items2, n))
    {
      retval = COLLISION_FOUND;
      mpz_add_ui(pack->perf.counter, pack->perf.counter, 1); // add to number of found sets
      if (!(*f)(pack->selected, n, data))
      {
        retval = SIGSTOP;
        goto ret;
      }
      hshtbl_insert(&pack->found, pack->items2, pack->selected, n, set_items_sqared_sum(pack->items2, n), n);
    }

    // cont:
    // remove entries from the match
    for (uint32_t i = 0; i < n - count; ++i)
    {
      uint32_t pos = node.items[i];
      pack->selected[pos] = 0;
    }

  } while ((node = table->arr[h = (h + 1) % table->capacity]).items != NULL && node.sum == sum);

ret:
  return retval;
}

/*
 * returns non-zero iff the entries in the n x n array `selected` indicate elements in `M` whose sum is magic
 */
uint8_t set_has_magic_sum(const uint8_t *selected, const pow_m_sqr M)
{
  const uint64_t mu = pow_m_sqr_sum_row(M, 0);
  uint64_t acc = 0;
  for (uint32_t i = 0; i < M.n; ++i)
    for (uint32_t j = 0; j < M.n; ++j)
      if (GET_AS_MAT(selected, i, j, M.n))
      {
        // printf("%"PRIu64"^%u + ", M_SQR_GET_AS_MAT(M, i, j), M.d);
        acc += ui_pow_ui(M_SQR_GET_AS_MAT(M, i, j), M.d);
      }

  //printf(" = %"PRIu64" != %"PRIu64"\n", acc, mu);

  return mu == acc;
}

/*
 * returns value is:
 * - positive if a set was found by random guessing or by collision
 * - zero if no set was found
 * - negative if signal was sent by the callback
 *
 * calls back only when set has magic value, in contrast with iterate_over_sets
 */
int8_t generate_random_set_with_magic_sum(state *pack, const pow_m_sqr M, const uint32_t r, const uint32_t s, set_callback f, void *data)
{
  const uint64_t n = r * s;

  uint64_t sum = 0;
  uint32_t count = 0;

  int8_t retval = NOT_FOUND;

  /*
  uint32_t test_entries1[] = {0x08, 0x17, 0x39, 0x3E,
                              0x64, 0x53, 0x77, 0x48,
                              0x8F, 0xBE, 0xA3, 0xB1,
                              0xDA, 0xD0, 0xE7, 0xED};

  uint32_t test_entries2[] = {0x2C, 0x0A, 0x26, 0x30,
                              0x7B, 0x68, 0x60, 0x73,
                              0x96, 0xB7, 0xAD, 0x84,
                              0xD8, 0xCD, 0xDC, 0xF0};
                              */

  while (count < n)
  {
#if 1
    int32_t selected_bloc = select_open_bloc(pack->selected, pack->col_sum, pack->row_sum, pack->open_blocs, r, s);
    if (selected_bloc < 0)
      // no valid blocs: abort with current search status (either NOT_FOUND or COLLISION_FOUND)
      return retval;

    uint32_t selected_bi = selected_bloc / r;
    uint32_t selected_bj = selected_bloc % r;

    ++(pack->row_sum[selected_bi]);
    ++(pack->col_sum[selected_bj]);

    // get one entry from the list we selected
    rel_item selected_entry = select_open_entry_in_bloc(pack->selected, pack->open_entries_in_bloc, selected_bi, selected_bj, r, s);
#else

    uint32_t selected_entry = 1;
    // printf("%"PRIu64".\t", pack->found->count);
    if (pack->found->count == 0)
      selected_entry = test_entries1[count];
    else if (pack->found->count == 1)
      selected_entry = test_entries2[count];
    else
      fprintf(stderr, "What ??? found.count = %"PRIu64"\n", pack->found->count);

    // printf("%u: %"PRIu64"\n", selected_entry, M_SQR_GET_AS_VEC(M, selected_entry));
#endif

    pack->selected[selected_entry] = 1;
    pack->items[count++] = selected_entry;

    sum += ui_pow_ui(M_SQR_GET_AS_VEC(M, selected_entry), M.d);

    if (sum > pack->mu)
      return NOT_FOUND;

    // printf("%u\n", count);
    if (count >= n)
      break; // we found a solution, dont insert nor check for collisions

    /*
     * do not insert partial rels which are n-1 elements long, as there is no way to find any new 1 element long rels, checking for collisions is enough
     */
    if (count < n - 1)
      hshtbl_insert(&(pack->tables[count - 1]), pack->items, pack->selected, count, sum, n); // -1 because pack->tables is zero indexed

    int8_t ret = check_if_set_can_be_formed_from_collision(pack, sum, r, s, count, f, data);
    if (ret > 0)
      // we found something: update retval
      retval = ret;
    if (ret < 0)
      return SIGSTOP;

    /*
     * if we reached down there, count must have gone up by one from the line:
     * ```items[count++] = selected_entry;```
     * Hence the loop will end
     */
  }

  // we randomly found a set

  if (set_has_magic_sum(pack->selected, M))
  {
    for (uint32_t k = 0, idx = 0; idx < n * n; ++idx)
    {
      if (pack->selected[idx])
        pack->items[k++] = idx;
    }

    if (!is_in_hshtbl(pack->found, pack->items, n))
    {
      // sum was magic
      hshtbl_insert(&pack->found, pack->items, pack->selected, n, set_items_sqared_sum(pack->items, n), n);
      mpz_add_ui(pack->perf.counter, pack->perf.counter, 1);
      if (!(*f)(pack->selected, n, data))
        return SIGSTOP;
    }
    return GUESS_FOUND;
  }

  // set with non-magic sum

  return retval;
}

#define MAX_ALLOWED_TRIES (100 * 1024)

void find_sets_collision_method(pow_m_sqr M, const uint32_t r, const uint32_t s, set_callback f, void *data)
{
  const size_t n = r * s;

  state pack = {0};
  init_state(&pack, r, s);
  pack.mu = pow_m_sqr_sum_row(M, 0); // magic sum
  pack.M = &M;

  uint64_t tries = 0;
  size_t *prev_counts = calloc((n - 1), sizeof(size_t));
  if (prev_counts == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  uint8_t refresh_frames = 0;

  int8_t ret;
  do
  {
    reset_state(&pack, r, s);
    ret = generate_random_set_with_magic_sum(&pack, M, r, s, f, data);
    ++tries;
    if (ret > 0)
      tries = 0;

    uint8_t changed = 0;
    uint64_t tables_tot_count = 0, tables_tot_capa = 0;
    for (uint32_t k = 0; k < (n - 1 - PREFILL_CAP); ++k)
    {
      /*
       * check if we changed, then update the counts hence we cannot break
       */
      const size_t c = pack.tables[k].count;
      if (c > prev_counts[k])
        changed = 1;
      prev_counts[k] = c;
      tables_tot_count += c;
      tables_tot_capa += pack.tables[k].capacity;
    }

    if (changed)
      tries = 0;

    if (tries > MAX_ALLOWED_TRIES)
    {
      fprintf(stderr, "No set (partial nor total) found for the last %u tries : aborting\n", MAX_ALLOWED_TRIES);
      break;
    }

#ifndef __NO_GUI__
    if (refresh_frames == 0)
    {
      clear();
      move(0, 0);
      for (uint32_t i = 0; i < (n - 1 - PREFILL_CAP); ++i)
      {
        printw("%7"PRIu64"/%7"PRIu64", ", pack.tables[i].count, pack.tables[i].capacity);
      }
      addch('\n');
      printw("tot: %"PRIu64"/ %"PRIu64": %.2f%%\n", tables_tot_count, tables_tot_capa, 100.0 * (double) tables_tot_count / (double) tables_tot_capa);

      char buff[256] = {0};
      gmp_snprintf(buff, 255, "tries, found: %7"PRIu64", %Zu/%u", tries, pack.perf.counter, REQUIERED_SETS);
      printw("%s\n", buff);
      print_perfw(&pack.perf, "sets");
      refresh();
    }
#else
    if (refresh_frames == 0)
    {
      printf("\rtot_count = %e", (double)tables_tot_count);
      fflush(stdout);
    }
#endif
    ++refresh_frames;
  } while (ret >= 0);

  free_state(pack, r, s);
  free(prev_counts);
  return;
}
