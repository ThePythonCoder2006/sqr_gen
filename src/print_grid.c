#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include "pow_m_sqr.h"

/*
 * max must be a non-NULL pointer to an array of length at least M.n
 * returns dwidth
 */
size_t pow_m_sqr_max_col_width(size_t *max, size_t *width, pow_m_sqr M)
{
  assert(max != NULL);
  char buff[32] = {0};

  for (uint64_t j = 0; j < M.n; ++j)
    for (uint64_t i = 0; i < M.n; ++i)
    {
      size_t l = snprintf(buff, 31, "%llu", M_SQR_GET_AS_MAT(M, i, j));
      if (l > max[j])
        max[j] = l;
    }

  size_t dwidth = snprintf(buff, 31, "%u", M.d);

  *width = 1; // start '|'

  for (uint64_t j = 0; j < M.n; ++j)
  {
    *width += max[j] + dwidth + 1; // + 1 for '|'
    // mvprintw(j, 30, "%llu", max[j]);
  }

  return dwidth;
}

/*
 * max must be a non-NULL pointer to an array of length at least M.n
 * returns dwidth
 */
size_t taxi_max_col_width(size_t *max, size_t *width, taxicab T)
{
  char buff[32] = {0};

  for (uint64_t j = 0; j < T.s; ++j)
    for (uint64_t i = 0; i < T.r; ++i)
    {
      size_t l = snprintf(buff, 31, "%llu", TAXI_GET_AS_MAT(T, i, j));
      if (l > max[j])
        max[j] = l;
    }

  size_t dwidth = snprintf(buff, 31, "%llu", T.d);

  *width = 1; // start '|'

  for (uint64_t j = 0; j < T.s; ++j)
  {
    *width += max[j] + dwidth + 1; // +1 for '|'
    // mvprintw(j, 30, "%llu", max[j]);
  }
  return dwidth;
}

#ifndef __DEBUG__
/*
 * if non-NULL, items must be of size n else it will not be used
 */
void mvpow_m_sqr_printw_highlighted(int y0, int x0, pow_m_sqr M, position *items1, position *items2, int BG_COLOR1, int BG_COLOR2)
{
  size_t *max = calloc(M.n, sizeof(size_t));
  if (max == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  size_t width = 0;
  size_t dwidth = pow_m_sqr_max_col_width(max, &width, M);

  uint8_t *selected1 = calloc(M.n * M.n, sizeof(uint8_t));
  if (selected1 == NULL)
  {
    fprintf(stderr, "[OOM] Buy mor RAM LOL!!\n");
    exit(1);
  }
  if (items1 != NULL)
    for (uint64_t k = 0; k < M.n; ++k)
      selected1[items1[k].i * M.n + items1[k].j] = 1;

  uint8_t *selected2 = calloc(M.n * M.n, sizeof(uint8_t));
  if (selected2 == NULL)
  {
    fprintf(stderr, "[OOM] Buy mor RAM LOL!!\n");
    exit(1);
  }
  if (items2 != NULL)
    for (uint64_t k = 0; k < M.n; ++k)
      selected2[items2[k].i * M.n + items2[k].j] = 1;

  init_pair(1, COLOR_WHITE, BG_COLOR1);
  init_pair(2, COLOR_WHITE, BG_COLOR2);

  int x = x0,
      y = y0;
  mvaddch(y, x, ACS_ULCORNER);
  mvvline(y + 1, x, 0, 3 * M.n);
  mvhline(y, x + 1, 0, width - 1);

  for (uint64_t i = 0; i < M.n; ++i)
  {
    x = x0 + 1;
    y += 1;
    for (uint64_t j = 0; j < M.n; ++j)
    {
      // x += max[j];
      if (selected1[i * M.n + j])
        attron(COLOR_PAIR(1));
      else if (selected2[i * M.n + j])
        attron(COLOR_PAIR(2));
      x += mvprintw(y, x, "%*u", max[j] + 1, M.d);
      attroff(COLOR_PAIR(1));
      attroff(COLOR_PAIR(2));
      if (i == 0)
      {
        if (j == M.n - 1)
          mvaddch(y - 1, x, ACS_URCORNER);
        else
          mvaddch(y - 1, x, ACS_TTEE);
      }
      else
      {
        if (j == M.n - 1)
          mvaddch(y - 1, x, ACS_RTEE);
        else
          mvaddch(y - 1, x, ACS_PLUS);
      }

      mvaddch(y, x, ACS_VLINE);
      mvaddch(y + 1, x, ACS_VLINE);

      if (j == 0)
      {
        mvhline(y + 2, x0 + 1, 0, width - 2);
        if (i == M.n - 1)
          mvaddch(y + 2, x0, ACS_LLCORNER);
        else
          mvaddch(y + 2, x0, ACS_LTEE);
      }

      if (i == M.n - 1)
      {
        if (j == M.n - 1)
          mvaddch(y + 2, x, ACS_LRCORNER);
        else
          mvaddch(y + 2, x, ACS_BTEE);
      }

      ++x;
    }

    x = x0 + 1;
    y += 1;

    for (uint64_t j = 0; j < M.n; ++j)
    {
      if (selected1[i * M.n + j])
        attron(COLOR_PAIR(1));
      else if (selected2[i * M.n + j])
        attron(COLOR_PAIR(2));
      x += mvprintw(y, x, "%*llu", (int)max[j], M_SQR_GET_AS_MAT(M, i, j));
      // x += dwidth + 1; // +1 for vertical seperator
      for (uint32_t _ = 0; _ < dwidth; ++_)
        x += mvprintw(y, x, " ");
      ++x; // for vertical separator
      attroff(COLOR_PAIR(1));
      attroff(COLOR_PAIR(2));
    }

    y += 1;
    x = x0;
  }

  move(y + 1, x0);

  free(max);

  return;
}

void mvpow_m_sqr_printw(int y0, int x0, pow_m_sqr M)
{
  mvpow_m_sqr_printw_highlighted(y0, x0, M, NULL, NULL, COLOR_BLACK, COLOR_BLACK);
  return;
}

void mvtaxicab_print(int y0, int x0, taxicab T)
{
  size_t *max = calloc(T.s, sizeof(size_t));
  if (max == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  size_t width = 0;
  size_t dwidth = taxi_max_col_width(max, &width, T);

  // for (uint64_t _ = 0; _ < width; ++_)
  //   putchar('-');
  // putchar('\n');

  int x = x0, y = y0;
  mvaddch(y, x, ACS_ULCORNER);
  mvvline(y + 1, x, 0, 3 * T.r);
  mvhline(y, x + 1, 0, width - 1);

  for (uint64_t i = 0; i < T.r; ++i)
  {
    x = x0 + 1;
    y += 1;
    for (uint64_t j = 0; j < T.s; ++j)
    {
      x += max[j];
      x += mvprintw(y, x, "%llu", T.d);
      if (i == 0)
      {
        if (j == T.s - 1)
          mvaddch(y - 1, x, ACS_URCORNER);
        else
          mvaddch(y - 1, x, ACS_TTEE);
      }
      else
      {
        if (j == T.s - 1)
          mvaddch(y - 1, x, ACS_RTEE);
        else
          mvaddch(y - 1, x, ACS_PLUS);
      }

      mvaddch(y, x, ACS_VLINE);
      mvaddch(y + 1, x, ACS_VLINE);

      if (j == 0)
      {
        mvhline(y + 2, x0 + 1, 0, width - 2);
        if (i == T.r - 1)
          mvaddch(y + 2, x0, ACS_LLCORNER);
        else
          mvaddch(y + 2, x0, ACS_LTEE);
      }

      if (i == T.r - 1)
      {
        if (j == T.s - 1)
          mvaddch(y + 2, x, ACS_LRCORNER);
        else
          mvaddch(y + 2, x, ACS_BTEE);
      }

      ++x;
    }

    x = x0 + 1;
    y += 1;

    for (uint64_t j = 0; j < T.s; ++j)
    {
      x += mvprintw(y, x, "%*llu", (int)max[j], TAXI_GET_AS_MAT(T, i, j));
      x += dwidth + 1; // +1 for vertical seperator
    }

    y += 1;
    x = x0;
  }

  move(y + 1, x0);

  free(max);

  return;
}

#endif

void pow_m_sqr_printf(pow_m_sqr M)
{
  size_t *max = calloc(M.n, sizeof(size_t));
  if (max == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  size_t width = 0;
  size_t dwidth = pow_m_sqr_max_col_width(max, &width, M);
  (void)width;

  for (uint64_t i = 0; i < M.n; ++i)
  {
    putchar('+');
    for (uint64_t j = 0; j < M.n; ++j)
    {
      for (uint64_t k = 0; k < max[j] + dwidth; ++k)
        putchar('-');
      putchar('+');
    }
    putchar('\n');

    putchar('|');
    for (uint64_t j = 0; j < M.n; ++j)
    {
      for (uint64_t k = 0; k < max[j]; ++k)
        putchar(' ');
      printf("%*u", (int)dwidth, M.d);
      putchar('|');
    }
    putchar('\n');

    putchar('|');
    for (uint64_t j = 0; j < M.n; ++j)
    {
      printf("%*llu", (int)max[j], M_SQR_GET_AS_MAT(M, i, j));
      for (uint64_t k = 0; k < dwidth; ++k)
        putchar(' ');
      putchar('|');
    }
    putchar('\n');
  }

  putchar('+');
  for (uint64_t j = 0; j < M.n; ++j)
  {
    for (uint64_t k = 0; k < max[j] + dwidth; ++k)
      putchar('-');
    putchar('+');
  }

  free(max);
  return;
}

void taxicab_printf(taxicab T)
{
  size_t *max = calloc(T.s, sizeof(size_t));
  if (max == NULL)
  {
    fprintf(stderr, "[OOM] Buy more RAM LOL!!\n");
    exit(1);
  }
  size_t width = 0;
  size_t dwidth = taxi_max_col_width(max, &width, T);

  // for (uint64_t _ = 0; _ < width; ++_)
  //   putchar('-');
  // putchar('\n');

  for (uint64_t i = 0; i < T.r; ++i)
  {
    putchar('+');
    for (uint64_t j = 0; j < T.s; ++j)
    {
      for (uint64_t k = 0; k < max[j] + dwidth; ++k)
        putchar('-');
      putchar('+');
    }
    putchar('\n');

    putchar('|');
    for (uint64_t j = 0; j < T.s; ++j)
    {
      for (uint64_t k = 0; k < max[j]; ++k)
        putchar(' ');
      printf("%*llu", (int)dwidth, T.d);
      putchar('|');
    }
    putchar('\n');

    putchar('|');
    for (uint64_t j = 0; j < T.s; ++j)
    {
      printf("%*llu", (int)max[j], TAXI_GET_AS_MAT(T, i, j));
      for (uint64_t k = 0; k < dwidth; ++k)
        putchar(' ');
      putchar('|');
    }
    putchar('\n');
  }

  putchar('+');
  for (uint64_t j = 0; j < T.s; ++j)
  {
    for (uint64_t k = 0; k < max[j] + dwidth; ++k)
      putchar('-');
    putchar('+');
  }

  free(max);

  return;
}