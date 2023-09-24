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

#include "upper_bound.h"
#include <limits.h>

int minmax(state_t *state, move_t *path, int len, int max_len) {
  if (len + state->n_bad > max_len) {
    return INT_MAX;
  }

  int n_stacks = state->n_stacks;
  int n_tiers = state->n_tiers;
  int *h = state->h;
  int *list = state->list;
  int **p = state->p;
  int **q = state->q;
  int **b = state->b;

  while (state->n_bad > 0) {
    while (is_retrievable(state)) {
      retrieve(state, len);
    }

    int src = list[0];
    int n_empty_slots = (n_stacks - 1) * n_tiers - (state->n_blocks - h[src]);
    if (b[src][h[src]] > n_empty_slots) {
      return INT_MAX;
    }

    int pri = p[src][h[src]];
    int dst;

    int i_max;
    int q_max;
    for (int i = n_stacks - 1;; i--) {
      int s = list[i];
      if (h[s] < n_tiers) {
        i_max = i;
        q_max = q[s][h[s]];
        break;
      }
    }

    if (pri > q_max && len + state->n_bad == max_len) {
      return INT_MAX;
    }

    if (pri <= q_max) {
      for (int i = 1;; i++) {
        int s = list[i];
        if (h[s] < n_tiers && pri <= q[s][h[s]]) {
          dst = s;
          break;
        }
      }
    } else {
      dst = list[i_max];
      if (h[dst] == n_tiers - 1) {
        for (int i = i_max - 1; i > 0; i--) {
          int s = list[i];
          if (h[s] < n_tiers) {
            dst = s;
            break;
          }
        }
      }
    }

    if (path != NULL) {
      path[len].p = pri;
      path[len].s = src;
      path[len].d = dst;
    }
    relocate(state, src, dst, ++len);
  }

  return len;
}
