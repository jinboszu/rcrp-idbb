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

#include "lowerbound.h"
#include <string.h>

/*
 * LB4
 */

#if 1
static int insertion_point(int *array, int len, int val) {
  if (len == 0 || array[len - 1] < val) {
    return len;
  }
  if (array[0] >= val) {
    return 0;
  }
  int l = 0;
  int r = len;
  while (l < r) {
    int m = (l + r) / 2;
    if (array[m] >= val) {
      r = m;
    } else {
      l = m + 1;
    }
  }
  return r;
}
#else
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
#endif

static void recursive_search(int *priority, int next, int n_bad, int *quality,
                             int len, int curr, int *best) {
  if (next == n_bad) {
    *best = curr;
    return;
  }

  int pos = insertion_point(quality, len, priority[next]);
  if (pos < len) {
    int backup = quality[pos];
    quality[pos] = priority[next];
    recursive_search(priority, next + 1, n_bad, quality, len, curr, best);
    quality[pos] = backup;
  }

  if (pos > 0 && curr + 1 < *best) {
    recursive_search(priority, next + 1, n_bad, quality, len, curr + 1, best);
  }
}

int internal_lb4(state_t *state, int k_limit, int *h, int *list, int *priority,
                 int *quality) {
  if (state->n_bad == 0 || k_limit == 0) {
    return state->n_bad;
  }

  int n_stacks = state->n_stacks;
  int n_tiers = state->n_tiers;
  int remain_bad = state->n_bad;
  item_t **conf = state->conf;

  memcpy(h, state->h, sizeof(int) * n_stacks);
  memcpy(list, state->list, sizeof(int) * n_stacks);

  int q_max;
  for (int i = n_stacks - 1;; i--) {
    int s = list[i];
    if (h[s] < n_tiers) {
      q_max = conf[s][h[s]].q;
      break;
    }
  }

  int k = 0;
  while (remain_bad > 0) {
    int s_min = list[0];
    int bad_cnt = conf[s_min][h[s_min]].b;

    int n_bad = 0;
    for (int t = h[s_min]; t > h[s_min] - bad_cnt; t--) {
      if (conf[s_min][t].p > q_max) {
        if (++k >= k_limit) {
          return state->n_bad + k;
        }
      } else {
        priority[n_bad++] = conf[s_min][t].p;
      }
    }

    if (n_bad > 1) {
      int len = 0;
      for (int i = 1; i < n_stacks; i++) {
        int s = list[i];
        if (h[s] < n_tiers) {
          quality[len++] = conf[s][h[s]].q;
        }
      }

      int best = n_bad - 1;
      recursive_search(priority, 0, n_bad, quality, len, 0, &best);
      if ((k += best) >= k_limit) {
        return state->n_bad + k;
      }
    }

    remain_bad -= bad_cnt;
    h[s_min] -= bad_cnt + 1;

    int i = 0;
    while (i < n_stacks - 1 && compare_stack(s_min, list[i + 1], conf, h) > 0) {
      list[i] = list[i + 1];
      i++;
    }
    list[i] = s_min;

    if (q_max < conf[s_min][h[s_min]].q) {
      q_max = conf[s_min][h[s_min]].q;
    }
  }

  return state->n_bad + k;
}
