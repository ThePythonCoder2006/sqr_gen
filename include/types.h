#ifndef __TYPES__
#define __TYPES__

#include <stdint.h>
#include <stdlib.h>

typedef struct pow_m_sqr_s
{
  uint32_t n, d;
  uint32_t *rows, *cols;
  uint64_t *arr;
} pow_m_sqr;

typedef struct highlighted_square_s
{
  uint32_t n, d;
  uint32_t *rows, *cols;
  struct highlighted_val_s
  {
    uint64_t val;
    uint8_t colour;
  } *arr;
} highlighted_square;

typedef struct
{
  uint32_t n;
  uint8_t *arr;
} latin_square;

/*
 * represents a (r, s, d)-taxicab
 * ie   a_{1, 1}^d + a_{1, 2}^d + ... + a_{1, s}^d
 *    = a_{2, 1}^d + a_{2, 2}^d + ... + a_{2, s}^d
 *    ...
 *    = a_{r, 1}^d + a_{r, 2}^d + ... + a_{r, s}^d
 * where a_{i, j} = arr[i * s + j]
 * r is the HEIGHT ie number of representations
 * s is the WIDTH ie the number of terms in each representation
 */
typedef struct
{
  uint32_t d, r, s;
  uint32_t *arr;
} taxicab;

typedef uint8_t rel_item;

/*
 * to store the position (i, j) set:
 * rel[k] = i*n + j
 * where 0 <= k < n the next availible index
 */
typedef rel_item* pos_rel;

/*
 * to store the position (i, j) set:
 * rel[i] = j
 */
typedef rel_item* x_y_rel;

/*
 * for each 0 <= idx < count
 * [   ] ... [   n*rel_items   ] ... [   ]
 *           ^
 *           items[idx]
 * can store either pos_rels or x_y_rels
 */
typedef struct da_sets_s
{
  rel_item **items;
  size_t count;
  size_t capacity;
  uint32_t n; // size of each set
} da_sets;

typedef uint8_t (*action)(latin_square*, uint32_t, latin_square*, uint32_t, void*);

extern const uint64_t A000479[];

#endif //__TYPES__
