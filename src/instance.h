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

#ifndef INSTANCE_H
#define INSTANCE_H

#include <stdio.h>

typedef struct {
  int s;
  int t;
  int p;
} block_t;

typedef struct {
  int n_stacks;
  int n_tiers;
  int n_blocks;
  int *h;
  int **p;
} instance_t;

instance_t *create_instance(int n_stacks, int n_tiers);
void free_instance(instance_t *inst);

instance_t *read_cvs_instance(FILE *fp, int n_tiers);
instance_t *read_zqlz_instance(FILE *fp);
void print_instance(FILE *fp, instance_t *inst);

#endif
