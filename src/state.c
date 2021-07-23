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

#include "state.h"
#include <stdlib.h>
#include <string.h>

state_t *create_state(int n_stacks, int n_tiers, bool has_head, bool has_body) {
  state_t *state = malloc(sizeof(state_t));
  state->n_stacks = n_stacks;
  state->n_tiers = n_tiers;
  if (has_head) {
    state->h = malloc(sizeof(int) * 4 * n_stacks);
    state->list = state->h + 1 * n_stacks;
    state->rank = state->h + 2 * n_stacks;
    state->last_change_time = state->h + 3 * n_stacks;
  }
  if (has_body) {
    state->conf = malloc(sizeof(item_t *) * n_stacks);
    state->conf[0] = malloc(sizeof(item_t) * n_stacks * (n_tiers + 1));
    for (int s = 1; s < n_stacks; s++) {
      state->conf[s] = state->conf[0] + s * (n_tiers + 1);
    }
  }
  return state;
}

void free_state(state_t *state, bool has_head, bool has_body) {
  if (has_head) {
    free(state->h);
  }
  if (has_body) {
    free(state->conf[0]);
    free(state->conf);
  }
  free(state);
}

void copy_state_head(state_t *dst_state, state_t *src_state) {
  dst_state->n_blocks = src_state->n_blocks;
  dst_state->n_bad = src_state->n_bad;
  memcpy(dst_state->h, src_state->h, sizeof(int) * 4 * dst_state->n_stacks);
}

void copy_state_body(state_t *dst_state, state_t *src_state) {
  memcpy(dst_state->conf[0], src_state->conf[0],
         sizeof(item_t) * dst_state->n_stacks * (dst_state->n_tiers + 1));
}

void copy_state(state_t *dst_state, state_t *src_state) {
  copy_state_head(dst_state, src_state);
  copy_state_body(dst_state, src_state);
}

void reuse_state_head(state_t *dst_state, state_t *src_state) {
  dst_state->n_blocks = src_state->n_blocks;
  dst_state->n_bad = src_state->n_bad;
  dst_state->h = src_state->h;
  dst_state->list = src_state->list;
  dst_state->rank = src_state->rank;
  dst_state->last_change_time = src_state->last_change_time;
}

void reuse_state_body(state_t *dst_state, state_t *src_state) {
  dst_state->conf = src_state->conf;
}

bool is_retrievable(state_t *state) {
  return state->n_blocks > 0 &&
         state->conf[state->list[0]][state->h[state->list[0]]].b == 0;
}

int compare_stack(int s1, int s2, item_t **conf, int *h) {
  return conf[s1][h[s1]].q - conf[s2][h[s2]].q;
}

static void adjust_left(state_t *state, int s) {
  int i = state->rank[s];
  while (i > 0 &&
         compare_stack(s, state->list[i - 1], state->conf, state->h) < 0) {
    state->list[state->rank[state->list[i - 1]] = i] = state->list[i - 1];
    i--;
  }
  state->list[state->rank[s] = i] = s;
}

static void adjust_right(state_t *state, int s) {
  int i = state->rank[s];
  while (i < state->n_stacks - 1 &&
         compare_stack(s, state->list[i + 1], state->conf, state->h) > 0) {
    state->list[state->rank[state->list[i + 1]] = i] = state->list[i + 1];
    i++;
  }
  state->list[state->rank[s] = i] = s;
}

void init_state(state_t *state, instance_t *inst) {
  state->n_blocks = inst->n_blocks;
  state->n_bad = 0;
  for (int s = 0; s < state->n_stacks; s++) {
    state->h[s] = inst->h[s];
    state->conf[s][0].b = 0;
    state->conf[s][0].p = __INT_MAX__;
    state->conf[s][0].q = __INT_MAX__;
    state->conf[s][0].l = 0;
    for (int t = 1; t <= state->h[s]; t++) {
      set_item(state, s, t, inst->p[s][t], 0);
      state->n_bad += state->conf[s][t].b > 0 ? 1 : 0;
    }
    state->list[state->rank[s] = s] = s;
    adjust_left(state, s);

    state->last_change_time[s] = 0;
  }
}

void print_state(FILE *fp, state_t *state) {
  for (int t = state->n_tiers; t >= 1; t--) {
    for (int s = 0; s < state->n_stacks; s++) {
      t > state->h[s] ? fprintf(fp, "[   ]")
                      : fprintf(fp, "[%3d]", state->conf[s][t].p);
    }
    fprintf(fp, "\n");
  }

  for (int s = 0; s < state->n_stacks; s++) {
    fprintf(fp, "-----");
  }
  fprintf(fp, "\n");

  for (int s = 0; s < state->n_stacks; s++) {
    fprintf(fp, " %3d ", s);
  }
  fprintf(fp, "\n");
}

void set_item(state_t *state, int s, int t, int p, int l) {
  item_t *item = &state->conf[s][t];
  item_t *below = &state->conf[s][t - 1];
  item->b = p <= below->q ? 0 : below->b + 1;
  item->p = p;
  item->q = p <= below->q ? p : below->q;
  item->l = l;
}

void relocate_out(state_t *state, int s, int l) {
  state->h[s]--;
  if (state->conf[s][state->h[s] + 1].b > 0) {
    state->n_bad--;
    adjust_left(state, s);
  } else {
    adjust_right(state, s);
  }
  state->last_change_time[s] = l;
}

void relocate_in(state_t *state, int d, int p, int l) {
  state->h[d]++;
  set_item(state, d, state->h[d], p, l);
  if (state->conf[d][state->h[d]].b > 0) {
    state->n_bad++;
    adjust_right(state, d);
  } else {
    adjust_left(state, d);
  }
  state->last_change_time[d] = l;
}

void relocate(state_t *state, int s, int d, int l) {
  relocate_out(state, s, l);
  relocate_in(state, d, state->conf[s][state->h[s] + 1].p, l);
}

void retrieve(state_t *state, int l) {
  int s = state->list[0];
  state->n_blocks--;
  state->h[s]--;
  adjust_right(state, s);
  state->last_change_time[s] = l;
}
