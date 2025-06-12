#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "curses.h"
#include "taxicab.h"
// #define __PERF_COUNTER_IMPLEMENTATION__
#include "perf_counter.h"

#include "find_taxicab.h"

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

static Node *hash_table[HASH_SIZE];
static uint64_t squares[MAX_BASE + 1];
static uint8_t squares_inited = 0;

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

int try_store_rep(uint64_t sum, Rep *rep)
{
  uint64_t h = hash_func(sum);
  Node *node = hash_table[h];

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
  node->sum = sum;
  node->rep_count = 1;
  node->reps = malloc(sizeof(Rep) * MAX_REP_PER_SUM);
  node->reps[0].terms = malloc(sizeof(int) * rep->s);
  memcpy(node->reps[0].terms, rep->terms, sizeof(int) * rep->s);
  node->reps[0].s = rep->s;
  node->next = hash_table[h];
  hash_table[h] = node;
  return 1;
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
  printf("Sum: %llu\n", sum);
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

void cleanup()
{
  for (int i = 0; i < HASH_SIZE; i++)
  {
    Node *node = hash_table[i];
    while (node)
    {
      for (int j = 0; j < node->rep_count; j++)
        free(node->reps[j].terms);
      free(node->reps);
      Node *tmp = node;
      node = node->next;
      free(tmp);
    }
    hash_table[i] = NULL;
  }
}

uint64_t find_terms(Rep *result_reps, int *terms, int r, int s);
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

  // int *terms = malloc(sizeof(int) * s);
  // Rep *result_reps = malloc(sizeof(Rep) * r);
  // uint64_t result_sum = find_terms(result_reps, terms, r, s);

  // print_result(result_sum, result_reps, r, s);

  taxicab T = {0};
  taxicab_init(&T, r, s, 2);
  find_taxicab(T);
// reps_to_taxicab(T, result_reps);
#ifndef __DEBUG__
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

  // free(result_reps);
  // free(terms);
  cleanup();
  return 0;
}

void reps_to_taxicab(taxicab T, Rep *reps)
{
  for (uint64_t i = 0; i < T.r; ++i)
    for (uint64_t j = 0; j < T.s; ++j)
    {
      // printf("%llu, %llu\n", i, j);
      TAXI_GET_AS_MAT(T, i, j) = reps[i].terms[j];
    }
}

/*
 * returns the sum
 * result_reps contains the found reps
 */
uint64_t find_terms(Rep *result_reps, int *terms, int r, int s)
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

    int count = try_store_rep(sum, &rep);
    if (count >= r)
    {
      uint64_t h = hash_func(sum);
      Node *node = hash_table[h];
      while (node && node->sum != sum)
        node = node->next;

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

void find_taxicab(taxicab T)
{
  if (!squares_inited)
  {
    init_squares();
    squares_inited = 1;
  }

  int *terms = malloc(sizeof(int) * T.s);
  Rep *result_reps = malloc(sizeof(Rep) * T.r);
  // uint64_t result_sum =
  (void)find_terms(result_reps, terms, T.r, T.s);

  reps_to_taxicab(T, result_reps);

  free(result_reps);
  free(terms);
  cleanup();
}