#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include "latin_squares.h"
#include "pow_m_sqr.h"

/*
 * P must be of length r and Q of length s
 * P must hold square of size s and Q must hold squares of size r
 */
void position_after_latin_square_permutation(uint32_t *ret_row, uint32_t *ret_col, rel_item row, rel_item col, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s)
{
  uint32_t i = row / r; // 0 <= i < s
  uint32_t j = col / s; // 0 <= j < r
  uint32_t u = row % r; // 0 <= u < r
  uint32_t v = col % s; // 0 <= v < s

  /*
   * a and b denote here (like in the rest of the codebase) (r, s, d)- and (s, r, d)-taxicabs respectively
   * M_{row, col} = [M_{i, j}]_{u, v} = ( a_{j, [P_standart]_{i, v} * b_{i, [Q_standart]_{j, u}} )^d
   *     in real cases (when P != P_standart)            ^---- j                     ^----- i
   * We want to know where the unique coefficient with that value ended up
   * ie we want to find the unique quatuor (i', j', u', v') such that:
   *  -> a_{j', P[j']_{i', v'}} = a_{j, [P_standart]_{i, v}}
   *  -> b_{i', Q[i']_{j', u'}} = b_{i, [Q_standart]_{j, u}}
   * and since a and b are taxicabs with distinct entries we obtain:
   *  -> j' = j
   *  -> P[j']_{i', v'} = [P_standart]_{i, v}
   *  -> i' = i
   *  -> Q[i']_{j', u'} = [Q_standart]_{j, u}
   * simplifying gives:
   *  -> (i', j') = (i, j)
   *  -> P[j]_{i, v'} = [P_standart]_{i, v}
   *  -> Q[i]_{j, u'} = [Q_standart]_{j, u}
   * We hence need to compare the i-th line from P[j] with the i-th line from the standart P latin square
   * and do the same with the j-th line of Q[i] and the j-th line from the standart Q latin square
   * In fact [P_standart]_{i, v} = (i + v) % s        (remember we have P_standart.n = s)
   * So we just need to find the v' for which this value is hit in P[j]_{i, v'}
   */

  const uint32_t P_standart_iv = ((i + v) % s);

  uint32_t v_prime;
  for (v_prime = 0; v_prime < s; ++v_prime)
  {
    if (GET_AS_MAT(P[j].arr, i, v_prime, P[j].n) == P_standart_iv)
      break;
    if (v_prime == s - 1)
    // last iteration and we did not hit it: absurd there must be a cell with value (i + v) % r in P[j], by construction of a latin square
    {
      fprintf(stderr, "[UNREACHABLE] P: %ux%u latin square with no %u in row %u\n", P[j].n, P[j].n, P_standart_iv, i);
      exit(1);
    }
  }

  const uint32_t Q_standart_ju = ((j + u) % r);

  uint32_t u_prime;
  for (u_prime = 0; u_prime < r; ++u_prime)
  {
    if (GET_AS_MAT(Q[i].arr, j, u_prime, Q[i].n) == Q_standart_ju)
      break;
    if (u_prime == r - 1)
    // last iteration and we did not hit it: absurd there must be a cell with value (j + u) % s in Q[i], by construction of a latin square
    {
      fprintf(stderr, "[UNREACHABLE] Q: %ux%u latin square with no %u in row %u\n", Q[i].n, Q[i].n, Q_standart_ju, j);
      exit(1);
    }
  }

  *ret_row = i * r + u_prime;
  *ret_col = j * s + v_prime;
  return;
}

uint8_t fall_on_different_line_after_latin_squares(rel_item *poses, latin_square *P, latin_square *Q, const uint32_t r, const uint32_t s)
{
  const uint64_t n = r * s;
  uint8_t *rows = calloc(n, sizeof(uint8_t));
  uint8_t *cols = calloc(n, sizeof(uint8_t));
  if (rows == NULL || cols == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  int retval = 1;
  for (uint64_t i = 0; i < n; ++i)
  {
    // printf("[2] %u, %u\n", poses[i].i, poses[i].j);
    uint32_t new_row = 0, new_col = 0;
    position_after_latin_square_permutation(&new_row, &new_col, i, poses[i], P, Q, r, s);
    if (rows[new_row] || cols[new_col])
    {
      retval = 0;
      goto ret;
    }
    rows[new_row] = 1;
    cols[new_col] = 1;
  }

ret:
  free(rows);
  free(cols);
  return retval;
}

void printf_rel(rel_item *rel, const size_t n)
{
  for (size_t i = 0; i < n; ++i)
    printf("(%"PRIu64", %u), ", i, rel[i]);

  return;
}

uint8_t rels_are_disjoint(rel_item *rel1, rel_item *rel2, const size_t n)
{
  // check for every row if they intersect
  // Could be replaced with a memcmp
  for (size_t i = 0; i < n; ++i)
    if (rel1[i] == rel2[i])
      return 0;

  return 1;
}

uint8_t rels_are_compatible(rel_item *rel1, rel_item *rel2, const size_t n)
{
  if (!rels_are_disjoint(rel1, rel2, n))
    return 0;

  /*
   * The compatibility condition is as follow:
   * initiate two counter, i = 0 and j = 0
   * for each column:
   *  - add one to i if the element of rel2 in this column is above the element of rel1 in the column
   *    if the elements from rel1 and rel2 collide: n is odd and this is the middle: ignore
   * for each row:
   *  - add one to the j if the element of rel2 in this row is to the left of the element of rel1 in the row
   *    if the elements from rel1 and rel2 collide: n is odd and this is the middle: ignore
   * FOR N EVEN:
   * rel1 and rel2 are compatible iff i = j = n/2 if n is even
   * FOR N ODD:
   * rel1 and rel2 are compatible iff i = j = (n - 1)/2 if n is odd
   */

  uint64_t left_count = 0;

  for (size_t i = 0; i < n; ++i)
    // check in row i if rel2 (blue) is before rel1 (yellow)
    left_count += rel2[i] < rel1[i];

  // n / 2 rounds correctly for both parity cases
  if (left_count != n / 2)
    return 0;

  uint64_t top_count = 0;
  for (size_t j = 0; j < n; ++j)
  {
    // find the rows where the elements in column j are in both rels
    // check if rel2 (blue) is higher than rel1 (yellow)
    size_t idx1_j = 0;
    for (; idx1_j < n && rel1[idx1_j] != j; ++idx1_j)
      ;
    size_t idx2_j = 0;
    for (; idx2_j < n && rel2[idx2_j] != j; ++idx2_j)
      ;
    top_count += idx2_j < idx1_j;
  }

  return top_count == n / 2;
}

typedef struct rel_item_pair_s{
  size_t x, y;
} rel_item_pair;

/*
 * diag[i] must be the column of the element of the set on line i
 * modifies M
 */
void permute_into_pow_m_sqr(pow_m_sqr *M, rel_item *diag1, rel_item *diag2)
{
  const uint64_t n = M->n;

  highlighted_square H = {0};
  highlighted_square_init(&H, n, M->d);
  highlighted_square_from_pow_m_sqr(&H, M, diag1, diag2);

  rel_item* diag1_inv = calloc(n, sizeof(rel_item));
  rel_item* sigma = calloc(n, sizeof(rel_item));
  uint8_t* visited = calloc(n, sizeof(uint8_t));
  rel_item_pair* pairs = calloc(n / 2, sizeof(rel_item_pair));

  if (diag1_inv == NULL || sigma == NULL || visited == NULL || pairs == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1); 
  }

  // preform the main diag
  for (size_t i = 0; i < n; ++i)
    diag1_inv[diag1[i]] = i;

  for (size_t i = 0; i < n; ++i)
    sigma[i] = diag1_inv[diag2[i]];

  // decompose sigma in transpositions
  size_t k = 0;
  size_t center = -1;
  for (size_t i = 0; i < n; ++i)
  {
    if (visited[i])
      continue;
    size_t j = sigma[i];
    visited[i] = 1;
    visited[j] = 1;
    if (i == j)
      center = i;
    else
      pairs[k++] = (rel_item_pair){.x=i, .y=j};
  }

  // organize the pairs in the permutation
  size_t left = 0, right = n - 1;
  for (k = 0; k < n/2; ++k)
  {
    H.rows[pairs[k].x] = left++;
    H.rows[pairs[k].y] = right--;
  }

  if (center > 0)
    H.rows[center] = left;

  // C = R o diag1^-1
  for (size_t i = 0; i < n; ++i)
    H.cols[i] = H.rows[diag1_inv[i]];


  pow_m_sqr_from_highlighted_square(M, NULL, NULL, &H);

  free(diag1_inv);
  free(sigma);
  free(visited);
  free(pairs);
  highlighted_square_clear(&H);

  return;
}

void highlighted_square_init(highlighted_square *ret, const uint32_t n, const uint32_t d)
{
  ret->n = n;
  ret->d = d;
  ret->cols = calloc(ret->n, sizeof(*ret->cols));
  ret->rows = calloc(ret->n, sizeof(*ret->rows));
  if (ret->cols == NULL || ret->rows == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }

  ret->arr = calloc(ret->n * ret->n, sizeof(*ret->arr));
  if (ret->arr == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  return;
}

void highlighted_square_clear(highlighted_square *ret)
{
  free(ret->cols);
  free(ret->rows);
  free(ret->arr);
  ret->n = 0;
  ret->d = 0;
  ret->cols = NULL;
  ret->rows = NULL;
  ret->arr = NULL;
  return;
}

/*
 * ret must be a non-NULL pointer to an inited highlighted_square
 */
void highlighted_square_from_pow_m_sqr(highlighted_square *ret, const pow_m_sqr *M, const rel_item *rel1, const rel_item *rel2)
{
  ret->n = M->n;
  ret->d = M->d;

  memcpy(ret->cols, M->cols, ret->n * sizeof(*ret->cols));
  memcpy(ret->rows, M->rows, ret->n * sizeof(*ret->rows));

  for (uint32_t i = 0; i < ret->n; ++i)
  {
    GET_AS_MAT(ret->arr, i, rel1[i], ret->n).colour = 1;
    GET_AS_MAT(ret->arr, i, rel2[i], ret->n).colour = 2;
    for (uint32_t j = 0; j < ret->n; ++j)
      // this has to be GET_AS_MAT and NOT M_SQR_GET_AS_MAT because of rows and cols, the permutation would be all wrong
      GET_AS_MAT(ret->arr, i, j, ret->n).val = GET_AS_MAT(M->arr, i, j, M->n);
  }

  return;
}

/*
 * ret must be a non-NULL pointer to an inited pow_m_sqr
 * sets the corresponding rels into rel1 and rel2
 */
void pow_m_sqr_from_highlighted_square(pow_m_sqr *ret, rel_item *rel1, rel_item *rel2, const highlighted_square *M)
{
  const uint32_t n = M->n;
  ret->n = M->n;
  ret->d = M->d;

  memcpy(ret->cols, M->cols, ret->n * sizeof(*ret->cols));
  memcpy(ret->rows, M->rows, ret->n * sizeof(*ret->rows));

  if (rel1 != NULL && rel2 != NULL)
  {
    memset(rel1, 0, n * sizeof(*rel1));
    memset(rel2, 0, n * sizeof(*rel2));
  }

  for (uint32_t i = 0; i < ret->n; ++i)
    for (uint32_t j = 0; j < ret->n; ++j)
    {
      // this has to be GET_AS_MAT and NOT M_SQR_GET_AS_MAT because of rows and cols, the permutation would be all wrong
      GET_AS_MAT(ret->arr, i, j, ret->n) = GET_AS_MAT(M->arr, i, j, M->n).val;

      if (rel1 != NULL && rel2 != NULL)
      {
        // this on the other hand has to M_SQR_GET_AS_MAT as we want to use the permutation
        if (M_SQR_GET_AS_MAT(*M, i, j).colour == 1)
          rel1[i] = j;
        if (M_SQR_GET_AS_MAT(*M, i, j).colour == 2)
          rel2[i] = j;
      }
    }

  return;
}

void rels_from_highlighted_square(rel_item *rel1, rel_item *rel2, const highlighted_square *M)
{
  const uint32_t n = M->n;
  memset(rel1, 0, n * sizeof(*rel1));
  memset(rel2, 0, n * sizeof(*rel2));
  for (uint32_t i = 0; i < M->n; ++i)
    for (uint32_t j = 0; j < M->n; ++j)
    {
      if (M_SQR_GET_AS_MAT(*M, i, j).colour == 1)
        rel1[i] = j;
      if (M_SQR_GET_AS_MAT(*M, i, j).colour == 2)
        rel2[i] = j;
    }

  return;
}

/*
 * rel1, rel2, rel1_inv MUST be at least n elements long
 */
uint8_t rels_are_diagonizable(rel_item* rel1, rel_item* rel2, rel_item* rel1_inv, rel_item* sigma, size_t n)
{
  for (size_t i = 0; i < n; ++i)
    rel1_inv[rel1[i]] = i;

  for (size_t i = 0; i < n; ++i)
    sigma[i] = rel1_inv[rel2[i]];

  size_t fixed = 0;
  for (size_t i = 0; i < n; ++i)
  {
    if (sigma[sigma[i]] != i)
      return 0;
    fixed += (sigma[i] == i);
  }

  return fixed == (n % 2);
}

