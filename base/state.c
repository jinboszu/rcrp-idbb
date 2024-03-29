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

state_t *malloc_state(int n_stacks, int n_tiers) {
  state_t *state = malloc(sizeof(state_t));
  state->n_stacks = n_stacks;
  state->n_tiers = n_tiers;
  state->h = malloc(sizeof(int) * 4 * n_stacks);
  state->list = state->h + 1 * n_stacks;
  state->rank = state->h + 2 * n_stacks;
  state->last_change_time = state->h + 3 * n_stacks;
  state->p = malloc(sizeof(int *) * 4 * n_stacks);
  state->q = state->p + 1 * n_stacks;
  state->b = state->p + 2 * n_stacks;
  state->l = state->p + 3 * n_stacks;
  state->p[0] = malloc(sizeof(int) * 4 * n_stacks * (n_tiers + 1));
  state->q[0] = state->p[0] + 1 * n_stacks * (n_tiers + 1);
  state->b[0] = state->p[0] + 2 * n_stacks * (n_tiers + 1);
  state->l[0] = state->p[0] + 3 * n_stacks * (n_tiers + 1);
  for (int s = 1; s < n_stacks; s++) {
    state->p[s] = state->p[0] + s * (n_tiers + 1);
    state->q[s] = state->q[0] + s * (n_tiers + 1);
    state->b[s] = state->b[0] + s * (n_tiers + 1);
    state->l[s] = state->l[0] + s * (n_tiers + 1);
  }
  return state;
}

void free_state(state_t *state) {
  free(state->h);
  free(state->p[0]);
  free(state->p);
  free(state);
}

void copy_state(state_t *dst_state, state_t *src_state) {
  dst_state->n_blocks = src_state->n_blocks;
  dst_state->n_bad = src_state->n_bad;
  memcpy(dst_state->h, src_state->h, sizeof(int) * 4 * dst_state->n_stacks);
  memcpy(dst_state->p[0], src_state->p[0],
         sizeof(int) * 4 * dst_state->n_stacks * (dst_state->n_tiers + 1));
}

bool is_retrievable(state_t *state) {
  return state->n_blocks > 0 &&
         state->b[state->list[0]][state->h[state->list[0]]] == 0;
}

static int compare(state_t *state, int s1, int s2) {
  return state->q[s1][state->h[s1]] - state->q[s2][state->h[s2]];
}

static void adjust_left(state_t *state, int s) {
  int i = state->rank[s];
  while (i > 0 && compare(state, s, state->list[i - 1]) < 0) {
    state->list[state->rank[state->list[i - 1]] = i] = state->list[i - 1];
    i--;
  }
  state->list[state->rank[s] = i] = s;
}

static void adjust_right(state_t *state, int s) {
  int i = state->rank[s];
  while (i < state->n_stacks - 1 && compare(state, s, state->list[i + 1]) > 0) {
    state->list[state->rank[state->list[i + 1]] = i] = state->list[i + 1];
    i++;
  }
  state->list[state->rank[s] = i] = s;
}

static void update_slot(state_t *state, int s, int t, int p, int l) {
  state->p[s][t] = p;
  if (t == 0 || p <= state->q[s][t - 1]) {
    state->q[s][t] = p;
    state->b[s][t] = 0;
  } else {
    state->q[s][t] = state->q[s][t - 1];
    state->b[s][t] = state->b[s][t - 1] + 1;
  }
  state->l[s][t] = l;
}

void init_state(state_t *state, instance_t *inst) {
  state->n_blocks = inst->n_blocks;
  state->n_bad = 0;
  for (int s = 0; s < state->n_stacks; s++) {
    state->h[s] = inst->h[s];
    update_slot(state, s, 0, inst->max_prio + 1, 0);
    for (int t = 1; t <= state->h[s]; t++) {
      update_slot(state, s, t, inst->p[s][t], 0);
      state->n_bad += state->b[s][t] > 0;
    }
    state->list[state->rank[s] = s] = s;
    adjust_left(state, s);

    state->last_change_time[s] = 0;
  }
}

static void move_out(state_t *state, int s, int l) {
  if (state->b[s][state->h[s]--] > 0) {
    state->n_bad--;
  } else {
    adjust_right(state, s);
  }
  state->last_change_time[s] = l;
}

static void move_in(state_t *state, int d, int p, int l) {
  update_slot(state, d, ++state->h[d], p, l);
  if (state->b[d][state->h[d]] > 0) {
    state->n_bad++;
  } else {
    adjust_left(state, d);
  }
  state->last_change_time[d] = l;
}

void relocate(state_t *state, int s, int d, int l) {
  int p = state->p[s][state->h[s]];
  move_out(state, s, l);
  move_in(state, d, p, l);
}

void retrieve(state_t *state, int l) {
  int s = state->list[0];
  state->n_blocks--;
  state->h[s]--;
  adjust_right(state, s);
  state->last_change_time[s] = l;
}
