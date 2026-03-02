#include <curses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#include <ncurses.h>
#include "pow_m_sqr.h"
#include "taxicab.h"
#include "perf_counter.h"
#include "types.h"
#include "find_taxicab.h"
#include "probas.h"
#include "arithmetic.h"

#define MAX_BASE 256 // largement assez grand pour notre utilisation, pour des taxicab de taille 4x11 au plus
#define HASH_SIZE 131071
#define MAX_REP_PER_SUM 128 // maximum representations stored per sum

typedef struct
{
  int *terms;
  int s;
} Rep;

typedef struct Node
{
  uint64_t sum;
  int rep_count;
  Rep *reps;
  struct Node *next;
} Node;

/* ── Instanciable hash table ─────────────────────────────────────────────── */

typedef struct
{
  Node *buckets[HASH_SIZE];
} HashTable;

static void ht_init(HashTable *ht)
{
  memset(ht->buckets, 0, sizeof(ht->buckets));
  return;
}

static void ht_cleanup(HashTable *ht)
{
  for (int i = 0; i < HASH_SIZE; i++)
  {
    Node *node = ht->buckets[i];
    while (node)
    {
      for (int j = 0; j < node->rep_count; j++)
        free(node->reps[j].terms);
      free(node->reps);
      Node *tmp = node;
      node = node->next;
      free(tmp);
    }
    ht->buckets[i] = NULL;
  }

  return;
}

/* ── Shared state ────────────────────────────────────────────────────────── */

static uint64_t squares[MAX_BASE + 1];
static uint8_t squares_inited = 0;

/* Legacy global table, kept for find_taxicab() / test() which are single-table callers */
static HashTable global_ht;

void init_squares()
{
  for (uint64_t i = 1; i <= MAX_BASE; i++)
    squares[i] = i * i;
}

static inline uint32_t hash_func(uint64_t x)
{
  // from splitmix64
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
  x = x ^ (x >> 31);
  return (uint32_t)(x % HASH_SIZE);
}

int cmp_int(const void *a, const void *b)
{
  int x = *(const int *)a;
  int y = *(const int *)b;
  return x - y;
}

int cmp_rep(const void *a, const void *b)
{
  Rep x = *(const Rep *)a;
  Rep y = *(const Rep *)b;
  return x.terms[0] - y.terms[0];
}

int rep_equal(const Rep *a, const Rep *b)
{
  if (a->s != b->s)
    return 0;
  for (int i = 0; i < a->s; i++)
    if (a->terms[i] != b->terms[i])
      return 0;
  return 1;
}

int sample_unique_terms(int s, int *out)
{
  int used[MAX_BASE + 1] = {0};
  int count = 0;
  while (count < s)
  {
    int n = 1 + rand() % MAX_BASE;
    if (!used[n])
    {
      used[n] = 1;
      out[count++] = n;
    }
  }
  return 1;
}

/* All hash operations now take an explicit HashTable* */

static int ht_try_store_rep(HashTable *ht, uint64_t sum, Rep *rep)
{
  uint64_t h = hash_func(sum);
  Node *node = ht->buckets[h];

  while (node)
  {
    if (node->sum == sum)
    {
      for (int i = 0; i < node->rep_count; i++)
      {
        if (rep_equal(&node->reps[i], rep))
          return 0; // duplicate
      }
      if (node->rep_count < MAX_REP_PER_SUM)
      {
        node->reps[node->rep_count].terms = malloc(sizeof(int) * rep->s);
        memcpy(node->reps[node->rep_count].terms, rep->terms, sizeof(int) * rep->s);
        node->reps[node->rep_count].s = rep->s;
        node->rep_count++;
      }
      return node->rep_count;
    }
    node = node->next;
  }

  node = malloc(sizeof(Node));
  if (node == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  node->sum = sum;
  node->rep_count = 1;
  node->reps = malloc(sizeof(Rep) * MAX_REP_PER_SUM);
  if (node->reps == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  node->reps[0].terms = malloc(sizeof(int) * rep->s);
  memcpy(node->reps[0].terms, rep->terms, sizeof(int) * rep->s);
  node->reps[0].s = rep->s;
  node->next = ht->buckets[h];
  ht->buckets[h] = node;
  return 1;
}

static Node *ht_find_node(HashTable *ht, uint64_t sum)
{
  Node *node = ht->buckets[hash_func(sum)];
  while (node && node->sum != sum)
    node = node->next;
  return node;
}

// Try all combinations of r representations using backtracking
int find_disjoint_reps(Rep *reps, int total, int r, Rep *out)
{
  int *stack = calloc(r, sizeof(int));
  int level = 0;

  while (level >= 0)
  {
    if (stack[level] >= total)
    {
      level--;
      if (level >= 0)
        stack[level]++;
      continue;
    }

    // Check if this level is compatible with previous choices
    int ok = 1;
    uint8_t used[MAX_BASE + 1] = {0};
    for (int i = 0; i < level; i++)
      for (int j = 0; j < reps[stack[i]].s; j++)
        used[reps[stack[i]].terms[j]] = 1;

    for (int j = 0; j < reps[stack[level]].s; j++)
    {
      if (used[reps[stack[level]].terms[j]])
      {
        ok = 0;
        break;
      }
    }

    if (!ok)
    {
      stack[level]++;
      continue;
    }

    if (level == r - 1)
    {
      // Found valid set
      for (int i = 0; i < r; i++)
        out[i] = reps[stack[i]];
      free(stack);
      return 1;
    }
    else
    {
      level++;
      stack[level] = stack[level - 1] + 1;
    }
  }

  free(stack);
  return 0;
}

void print_result(uint64_t sum, Rep *reps, int r, int s)
{
  printf("\nFound (%d, %d, 2)-taxicab number with globally disjoint terms:\n", r, s);
  printf("Sum: %"PRIu64"\n", sum);
  for (int i = 0; i < r; i++)
  {
    printf("  = ");
    for (int j = 0; j < reps[i].s; j++)
    {
      printf("%d^2", reps[i].terms[j]);
      if (j + 1 < reps[i].s)
        printf(" + ");
    }
    printf("\n");
  }
}

/* Legacy wrapper kept for callers that don't care about multi-table usage */
void cleanup(void)
{
  ht_cleanup(&global_ht);
}

uint64_t find_terms_ht(HashTable *ht, Rep *result_reps, int *terms, int r, int s);
void reps_to_taxicab(taxicab T, Rep *reps);

int test(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <r> <s>\n", argv[0]);
    return 1;
  }

  int r = atoi(argv[1]);
  int s = atoi(argv[2]);

  if (r < 2 || s < 2 || r * s > MAX_BASE)
  {
    fprintf(stderr, "Invalid input. Must have r, s >= 2 and r*s <= %d\n", MAX_BASE);
    return 1;
  }

  srand(time(NULL));
  if (!squares_inited)
  {
    init_squares();
    squares_inited = 1;
  }

  taxicab T = {0};
  taxicab_init(&T, r, s, 2);
  find_taxicab(T);
#ifndef __NO_GUI__
  initscr();
  mvtaxicab_print(0, 0, T);
  printw("is%s a (%d, %d, 2)-taxicab\n", is_taxicab(T) ? "" : " not", r, s);
  refresh();
  getch();
  endwin();
#else
  taxicab_printf(T);
  printf("\nis%s a (%d, %d, 2)-taxicab\n", is_taxicab(T) ? "" : " not", r, s);
#endif
  taxicab_clear(&T);

  cleanup();
  return 0;
}

void reps_to_taxicab(taxicab T, Rep *reps)
{
  for (uint64_t i = 0; i < T.r; ++i)
    for (uint64_t j = 0; j < T.s; ++j)
      TAXI_GET_AS_MAT(T, i, j) = reps[i].terms[j];
}

/*
 * Core search: uses the supplied HashTable so callers control isolation.
 * Returns the taxicab sum; result_reps is filled on success.
 */
uint64_t find_terms_ht(HashTable *ht, Rep *result_reps, int *terms, int r, int s)
{
  Rep rep = {.terms = terms, .s = s};
  uint64_t result_sum = 0;

  while (1)
  {
    sample_unique_terms(s, rep.terms);
    qsort(rep.terms, s, sizeof(int), cmp_int);

    uint64_t sum = 0;
    for (int i = 0; i < s; i++)
      sum += squares[rep.terms[i]];

    int count = ht_try_store_rep(ht, sum, &rep);
    if (count >= r)
    {
      Node *node = ht_find_node(ht, sum);
      if (node && find_disjoint_reps(node->reps, node->rep_count, r, result_reps))
      {
        result_sum = sum;
        break;
      }
    }
  }

  qsort(result_reps, r, sizeof(Rep), cmp_rep);
  return result_sum;
}

/* Convenience wrapper that uses the legacy global table (unchanged behaviour) */
uint64_t find_terms(Rep *result_reps, int *terms, int r, int s)
{
  return find_terms_ht(&global_ht, result_reps, terms, r, s);
}

void find_taxicab(taxicab T)
{
  if (!squares_inited)
  {
    init_squares();
    squares_inited = 1;
  }

  int *terms = malloc(sizeof(int) * T.s);
  Rep *result_reps = malloc(sizeof(Rep) * T.r);
  if (terms == NULL || result_reps == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  (void) find_terms(result_reps, terms, T.r, T.s);

  reps_to_taxicab(T, result_reps);

  free(result_reps);
  free(terms);
  cleanup();
}

/*
 * Find two taxicabs a (r×s) and b (s×r) whose cross-products are distinct
 * and whose expected number of diagonal squares exceeds p.
 *
 * Each taxicab gets its own HashTable so accumulated work is never shared
 * between the two searches, eliminating the cross-contamination bug while
 * preserving hash-table warmth across retries of the outer loop.
 *
 * Returns 1 when a valid pair is found.
 */
int find_taxicabs_proba(taxicab a, taxicab b, double p)
{
  if (a.r != b.s || a.s != b.r || a.d != b.d)
  {
    fprintf(stderr, "[ERROR] taxicab sizes or exponent mismatch: a(%u, %u, %u) != (b(%u, %u, %u))^T\n", a.r, a.s, a.d, b.r, b.s, b.d);
    return 0;
  }

  const uint32_t r = a.r, s = a.s;
  const size_t n = r * s;

  if (!squares_inited)
  {
    init_squares();
    squares_inited = 1;
  }

  int *terms_a = malloc(sizeof(int) * a.s);
  Rep *result_reps_a = malloc(sizeof(Rep) * a.r);
  int *terms_b = malloc(sizeof(int) * b.s);
  Rep *result_reps_b = malloc(sizeof(Rep) * b.r);

  if (terms_a == NULL || terms_b == NULL || result_reps_a == NULL || result_reps_b == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  /* One independent table per taxicab — they never see each other's entries. */
  HashTable ht_a, ht_b;
  ht_init(&ht_a);
  ht_init(&ht_b);

  pow_m_sqr M = {0};
  pow_m_sqr_init(&M, n, a.d);
  double p_latin = 0;
  double max_p_latin = 0;

  uint16_t refresh_frames = 0;
  do
  {
    ++refresh_frames;
    (void) find_terms_ht(&ht_a, result_reps_a, terms_a, r, s);
    (void) find_terms_ht(&ht_b, result_reps_b, terms_b, s, r);

    reps_to_taxicab(a, result_reps_a);
    reps_to_taxicab(b, result_reps_b);
    if (!taxicab_cross_products_are_distinct(a, b)) continue;

    pow_semi_m_sqr_from_taxicab(M, a, b, NULL, NULL);
    p_latin = proba_with_latin_square(M, r, s);
    if (p_latin > max_p_latin)
      max_p_latin = p_latin;

    if ((refresh_frames & 0x3f) == 0)
    {
#ifndef __NO_GUI__
      clear();
      mvprintw(0, 0, "p_latin: %lf, max: %lf\n", p_latin, max_p_latin);
      refresh();
#else
      printf("\rp_latin: %lf, max: %lf\n", p_latin, max_p_latin);
#endif
    }
  } while (p_latin < p);

  (void) taxicab_reduce(a);
  (void) taxicab_reduce(b);
  
  ht_cleanup(&ht_a);
  ht_cleanup(&ht_b);

  free(result_reps_a);
  free(terms_a);
  free(result_reps_b);
  free(terms_b);

  return 1;
}

/*
 * Divide every coefficient of T by the GCD of all its coefficients in-place.
 * If all coefficients are 0 the function is a no-op.
 * Returns the GCD that was applied (1 means nothing changed).
 */
int taxicab_reduce(taxicab T)
{
  /* Compute the global GCD across all r*s entries. */
  int g = 0;
  for (uint32_t i = 0; i < T.r; ++i)
    for (uint32_t j = 0; j < T.s; ++j)
      g = gcd(g, TAXI_GET_AS_MAT(T, i, j));

  if (g <= 1)
    return g; /* Nothing to do — already primitive, or all-zero. */

  for (uint32_t i = 0; i < T.r; ++i)
    for (uint32_t j = 0; j < T.s; ++j)
      TAXI_GET_AS_MAT(T, i, j) /= g;

  return g;
}
