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

#include "heuristic.h"

bool heuristic(state_t *state, relocation_t *path, int len, int max_len,
               int *new_len) {
  if (len + state->n_bad > max_len) {
    return false;
  }

  int n_stacks = state->n_stacks;
  int n_tiers = state->n_tiers;
  int *h = state->h;
  int *list = state->list;
  item_t **conf = state->conf;

  while (state->n_bad > 0) {
    while (is_retrievable(state)) {
      retrieve(state, len);
    }

    int src = list[0];
    int empty_slots = (n_stacks - 1) * n_tiers - (state->n_blocks - h[src]);
    if (conf[src][h[src]].b > empty_slots) {
      return false;
    }

    int pri = conf[src][h[src]].p;
    int dst;

    int i_max;
    int q_max;
    for (int i = n_stacks - 1;; i--) {
      int s = list[i];
      if (h[s] < n_tiers) {
        i_max = i;
        q_max = conf[s][h[s]].q;
        break;
      }
    }

    /*
     * If a bad-good relocation is impossible while the path length limitation
     * is already reached, ...
     */
    if (pri > q_max && len + state->n_bad == max_len) {
      return false;
    }

    /*
     * If a bad-good relocation for the topmost item is possible, ...
     */
    if (pri <= q_max) {
      /*
       * Find a bad-good relocation
       */
      for (int i = 1;; i++) {
        int s = list[i];
        if (h[s] < n_tiers && pri <= conf[s][h[s]].q) {
          dst = s;
          break;
        }
      }
    } else {
      /*
       * Find a bad-bad relocation for `pri`
       */
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

    relocate(state, src, dst, len + 1);

    if (path != NULL) {
      path[len].pri = pri;
      path[len].src = src;
      path[len].dst = dst;
    }
    len++;
  }

  if (new_len != NULL) {
    *new_len = len;
  }

  return true;
}
