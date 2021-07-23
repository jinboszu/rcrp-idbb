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

#ifndef STATE_H
#define STATE_H

#include "instance.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  int b;
  int p;
  int q;
  int l;
} item_t;

typedef struct {
  int n_stacks;
  int n_tiers;
  int n_blocks;
  int n_bad;
  int *h;
  int *list;
  int *rank;
  int *last_change_time;
  item_t **conf;
} state_t;

state_t *create_state(int n_stacks, int n_tiers, bool has_head, bool has_body);
void free_state(state_t *state, bool has_head, bool has_body);

void copy_state_head(state_t *dst_state, state_t *src_state);
void copy_state_body(state_t *dst_state, state_t *src_state);
void copy_state(state_t *dst_state, state_t *src_state);
void reuse_state_head(state_t *dst_state, state_t *src_state);
void reuse_state_body(state_t *dst_state, state_t *src_state);

bool is_retrievable(state_t *state);

int compare_stack(int s1, int s2, item_t **conf, int *h);

void init_state(state_t *state, instance_t *inst);
void print_state(FILE *fp, state_t *state);

void set_item(state_t *state, int s, int t, int p, int l);

void relocate_out(state_t *state, int s, int l);
void relocate_in(state_t *state, int d, int p, int l);
void relocate(state_t *state, int sn, int d, int l);
void retrieve(state_t *state, int l);

#endif
