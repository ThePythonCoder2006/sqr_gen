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

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "find_sets.h"
#include "pow_m_sqr.h"

// forward declaration
uint64_t ui_pow_ui(uint64_t x, uint64_t n);

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

  if (!GET_AS_MAT(d->selected, current_row, current_col, d->n) && d->sum_row[block_row] < d->r && d->sum_col[block_col] < d->s)
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

#define HSHTBL_SIZE 1003 // big prime number

typedef struct hshtbl_node_s
{
  uint64_t sum;
  uint32_t *items;
  struct hshtbl_node_s *next;
} hshtbl_node;

typedef hshtbl_node *hshtbl[HSHTBL_SIZE];

/*
 * returns an array of n - 1 hshtbls as there is no need for a hshtbl for sets 16 entries long
 * hshtbl with index k will hold set of postions with k + 1 entries
 */
hshtbl *init_hshtbls(const uint32_t n)
{
  hshtbl *table = calloc(n - 1, sizeof(hshtbl));
  if (table == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  return table;
}

void free_hshtbls(hshtbl *table, const uint32_t n)
{
  for (uint32_t k = 0; k < n; ++k)
    for (uint32_t i = 0; i < HSHTBL_SIZE; i++)
    {
      hshtbl_node *node = table[k][i];
      while (node)
      {
        free(node->items);
        hshtbl_node *tmp = node;
        node = node->next;
        free(tmp);
      }
      table[k][i] = NULL;
    }

  free(table);
}

static inline uint32_t hash_func(uint64_t x)
{
  // from splitmix64
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  x = x ^ (x >> 31);
  return (uint32_t)(x % HSHTBL_SIZE);
}

/*
 * modifies table
 */
void hshtbl_insert(hshtbl table, const uint32_t *items, const uint32_t count, const uint64_t sum)
{
  uint64_t h = hash_func(sum);

  hshtbl_node *node = table[h];
  while (node)
  {
    if (memcmp(node->items, items, count * sizeof(uint32_t)) == 0)
      return; // duplicate
    node = node->next;
  }

  node = malloc(sizeof(hshtbl_node));
  if (node == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  node->sum = sum;
  node->items = calloc(count, sizeof(uint32_t));
  if (node->items == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  memcpy(node->items, items, count * sizeof(uint32_t));

  node->next = table[h];
  table[h] = node;
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

int8_t are_compatible_sets(const uint8_t *selected, const uint32_t *items, uint32_t *row_sum, uint32_t *col_sum, const uint32_t r, const uint32_t s, const uint32_t count)
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

/*
 * returns negative value if search should stop
 */
int8_t check_if_set_can_be_formed_from_collision(hshtbl table, uint8_t *selected, const uint32_t *row_sum, const uint32_t *col_sum, const uint64_t sum, const uint64_t mu,
                                                 const uint32_t r, const uint32_t s, const uint32_t count, set_callback f, void *data)
{
  hshtbl_node *node = table[hash_func(mu - sum)];
  if (node == NULL)
    return 0;

  const uint32_t n = r * s;

  uint32_t *row_sum_copy = calloc(s, sizeof(uint32_t));
  uint32_t *col_sum_copy = calloc(r, sizeof(uint32_t));
  if (row_sum_copy == NULL || col_sum_copy == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  // found a least match if there are no repeated entries and the union respects the sum conditions
  do
  {
    // reset the row/col sum arrays
    memcpy(row_sum_copy, row_sum, s * sizeof(uint32_t));
    memcpy(col_sum_copy, col_sum, r * sizeof(uint32_t));

    if (!are_compatible_sets(selected, node->items, row_sum_copy, col_sum_copy, r, s, count))
      continue;

    // add entries from the match
    for (uint32_t i = 0; i < n - count; ++i)
      selected[node->items[i]] = 1;

    if (!(*f)(selected, n, data))
      return -1;

    // remove entries from the match
    for (uint32_t i = 0; i < n - count; ++i)
      selected[node->items[i]] = 0;
  } while ((node = node->next) != NULL);

  return 0;
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
        acc += ui_pow_ui(M_SQR_GET_AS_MAT(M, i, j), M.d);

  return mu == acc;
}

/*
 * returns value is:
 * - positive if a set was found by random guessing
 * - zero if no set was found or some were found by collisions
 * - negative if signal was sent by the callback
 *
 * calls back only when set has magic value, in contrast with iterate_over_sets
 */
int8_t generate_random_set_with_magic_sum(hshtbl *table, const pow_m_sqr M, const uint32_t r, const uint32_t s, set_callback f, void *data)
{
  const uint64_t n = r * s;
  uint8_t *selected = calloc(n * n, sizeof(uint8_t));
  uint32_t *row_sum = calloc(s, sizeof(uint32_t));
  uint32_t *col_sum = calloc(r, sizeof(uint32_t));

  uint32_t *items = calloc(n, sizeof(uint32_t));

  // lists of entry indices of open blocs/cells, there are at most r * s
  uint32_t *open_blocs = calloc(r * s, sizeof(uint32_t));
  uint32_t *open_entries_in_bloc = calloc(r * s, sizeof(uint32_t));

  if (selected == NULL || row_sum == NULL || col_sum == NULL || items == NULL || open_blocs == NULL || open_entries_in_bloc == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  const uint64_t mu = pow_m_sqr_sum_row(M, 0); // magic sum
  uint64_t sum = 0;
  uint32_t count = 0;

  int8_t retval = 0;

  while (count < n)
  {
    int32_t selected_bloc = select_open_bloc(selected, col_sum, row_sum, open_blocs, r, s);
    if (selected_bloc < 0)
    // no valid blocs: abort
    {
      retval = 0;
      goto ret;
    }
    uint32_t selected_bi = selected_bloc / r;
    uint32_t selected_bj = selected_bloc % r;

    ++(row_sum[selected_bi]);
    ++(col_sum[selected_bj]);

    // get one entry from the list we selected
    uint32_t selected_entry = select_open_entry_in_bloc(selected, open_entries_in_bloc, selected_bi, selected_bj, r, s);
    // uint32_t selected_i = selected_entry / s;
    // uint32_t selected_j = selected_entry % s;

    selected[selected_entry] = 1;
    items[count++] = selected_entry;

    sum += ui_pow_ui(M_SQR_GET_AS_VEC(M, selected_entry), M.d);

    // printf("%u\n", count);
    if (count >= n)
      break; // we found a solution, dont insert nor check for collisions

    hshtbl_insert(table[count - 1], items, count, sum); // -1 because hshtbl is zero indexed

    if (check_if_set_can_be_formed_from_collision(table[n - count - 1], selected, row_sum, col_sum, sum, mu, r, s, count, f, data) < 0)
    {
      retval = -1;
      goto ret;
    }

    /*
     * if we reached down there, count must have gone up by one from the line:
     * ```items[count++] = selected_entry;```
     * Hence the loop will end
     */
  }

  // we randomly found a set

  if (set_has_magic_sum(selected, M))
  {
    // sum was magic
    if (!(*f)(selected, n, data))
    {
      retval = -1;
      goto ret;
    }
    retval = 1;
    goto ret;
  }

  // sum with non-magic sum
  retval = 0;

ret:
  free(selected);
  free(row_sum);
  free(col_sum);
  free(items);
  free(open_blocs);
  free(open_entries_in_bloc);
  return retval;
}

void find_sets_collision_method(pow_m_sqr M, const uint32_t r, const uint32_t s, set_callback f, void *data)
{
  hshtbl *table = init_hshtbls(r * s);
  while (generate_random_set_with_magic_sum(table, M, r, s, f, data) >= 0)
    ;
  return;
}