/*
 * Copyright (c) 2021 Bo Jin <jinbostar@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "lower_bound.h"
#include <stdlib.h>
#include <string.h>

/*
 * LB4
 */

static int compare_int(const void *a, const void *b) {
  return *(int *)a - *(int *)b;
}

static int insertion_point(int *array, int len, int val) {
  if (len == 0 || array[len - 1] < val) {
    return len;
  }
  int i = 0;
  while (array[i] < val) {
    i++;
  }
  return i;
}

static int enumerate(int *priority, int next, int n_bad, int *quality, int len,
                     int curr, int best) {
  if (next == n_bad) {
    return curr;
  }

  int pos = insertion_point(quality, len, priority[next]);
  if (pos < len) {
    int backup = quality[pos];
    quality[pos] = priority[next];
    best = enumerate(priority, next + 1, n_bad, quality, len, curr, best);
    quality[pos] = backup;
  }

  if (pos > 0 && curr + 1 < best) {
    best = enumerate(priority, next + 1, n_bad, quality, len, curr + 1, best);
  }

  return best;
}

int lb4(state_t *state, int *h, int *quality, int *priority) {
  if (state->n_bad == 0) {
    return state->n_bad;
  }

  int n_stacks = state->n_stacks;
  int n_tiers = state->n_tiers;
  int remain = state->n_bad;
  int **p = state->p;
  int **q = state->q;
  int **b = state->b;

  memcpy(h, state->h, sizeof(int) * n_stacks);

  int k = 0;
  while (remain > 0) {
    int s_min = -1;
    int s_max = -1;
    for (int s = 0; s < n_stacks; s++) {
      if (s_min == -1 || q[s_min][h[s_min]] > q[s][h[s]]) {
        s_min = s;
      }
      if (h[s] < n_tiers) {
        if (s_max == -1 || q[s_max][h[s_max]] < q[s][h[s]]) {
          s_max = s;
        }
      }
    }

    int bad_cnt = b[s_min][h[s_min]];

    int n_bad = 0;
    for (int t = h[s_min]; t > h[s_min] - bad_cnt; t--) {
      if (p[s_min][t] > q[s_max][h[s_max]]) {
        k++;
      } else {
        priority[n_bad++] = p[s_min][t];
      }
    }

    if (n_bad > 1) {
      int len = 0;
      for (int s = 0; s < n_stacks; s++) {
        if (s != s_min && h[s] < n_tiers) {
          quality[len++] = q[s][h[s]];
        }
      }
      qsort(quality, len, sizeof(int), compare_int);

      k += enumerate(priority, 0, n_bad, quality, len, 0, n_bad - 1);
    }

    remain -= bad_cnt;
    h[s_min] -= bad_cnt + 1;
  }

  return state->n_bad + k;
}
