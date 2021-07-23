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

#include "instance.h"
#include <stdlib.h>
#include <string.h>

instance_t *create_instance(int n_stacks, int n_tiers) {
  instance_t *inst = malloc(sizeof(instance_t));
  inst->n_stacks = n_stacks;
  inst->n_tiers = n_tiers;
  inst->h = malloc(sizeof(int) * n_stacks);
  inst->p = malloc(sizeof(int *) * n_stacks);
  inst->p[0] = malloc(sizeof(int) * n_stacks * (n_tiers + 1));
  for (int s = 1; s < n_stacks; s++) {
    inst->p[s] = inst->p[0] + s * (n_tiers + 1);
  }
  return inst;
}

void free_instance(instance_t *inst) {
  free(inst->h);
  free(inst->p[0]);
  free(inst->p);
  free(inst);
}

instance_t *read_cvs_instance(FILE *fp, int n_tiers) {
  instance_t *inst = NULL;

  char buf[BUFSIZ];
  int line = 0;
  char *iter, *next;

  do {
    line++;
    fgets(buf, BUFSIZ, fp);
    iter = buf + strspn(buf, " \t");
  } while (*iter == '#' || *iter == '\0');

  int n_stacks;
  int n_blocks;

  n_stacks = strtol(iter, &next, 10);
  if (iter == next) {
    fprintf(stderr, "Failed to read `n_stacks` in line %d: %s\n", line, buf);
    goto return_instance;
  }
  iter = next;
  n_blocks = strtol(iter, &next, 10);
  if (iter == next) {
    fprintf(stderr, "Failed to read `n_blocks` in line %d: %s\n", line, buf);
    goto return_instance;
  }
  iter = next;

  int *height = malloc(sizeof(int) * n_stacks);
  block_t *blocks = malloc(sizeof(block_t) * n_blocks);
  int num = 0;
  int max_height = 0;

  for (int s = 0; s < n_stacks; s++) {
    do {
      line++;
      fgets(buf, BUFSIZ, fp);
      iter = buf + strspn(buf, " \t");
    } while (*iter == '#' || *iter == '\0');

    height[s] = strtol(iter, &next, 10);
    if (iter == next) {
      fprintf(stderr, "Failed to read `h[%d]` in line %d: %s\n", s + 1, line,
              buf);
      goto free_resource;
    }
    iter = next;

    for (int t = 1; t <= height[s]; t++) {
      int p = strtol(iter, &next, 10);
      if (iter == next) {
        fprintf(stderr, "Failed to read `p[%d][%d]` in line %d: %s\n", s + 1, t,
                line, buf);
        goto free_resource;
      }
      iter = next;
      blocks[num].s = s;
      blocks[num].t = t;
      blocks[num].p = p;
      num++;
    }
    if (max_height < height[s]) {
      max_height = height[s];
    }
  }

  inst = create_instance(n_stacks, n_tiers);
  inst->n_blocks = n_blocks;
  memcpy(inst->h, height, sizeof(int) * n_stacks);
  for (num = 0; num < n_blocks; num++) {
    int s = blocks[num].s;
    int t = blocks[num].t;
    int p = blocks[num].p;
    inst->p[s][t] = p;
  }

free_resource:
  free(height);
  free(blocks);

return_instance:
  return inst;
}

instance_t *read_zqlz_instance(FILE *fp) {
  instance_t *inst = NULL;

  char buf[BUFSIZ];
  int line = 0;
  char *iter, *next;

  do {
    line++;
    fgets(buf, BUFSIZ, fp);
    iter = buf + strspn(buf, " \t");
  } while (*iter == '#' || *iter == '\0');

  int n_stacks;
  int n_tiers;
  int n_blocks;

  n_stacks = strtol(iter, &next, 10);
  if (iter == next) {
    fprintf(stderr, "Failed to read `n_stacks` in line %d: %s\n", line, buf);
    goto return_instance;
  }
  iter = next;
  n_tiers = strtol(iter, &next, 10);
  if (iter == next) {
    fprintf(stderr, "Failed to read `n_tiers` in line %d: %s\n", line, buf);
    goto return_instance;
  }
  iter = next;
  n_blocks = strtol(iter, &next, 10);
  if (iter == next) {
    fprintf(stderr, "Failed to read `n_blocks` in line %d: %s\n", line, buf);
    goto return_instance;
  }
  iter = next;

  int *height = malloc(sizeof(int) * n_stacks);
  block_t *blocks = malloc(sizeof(block_t) * n_blocks);
  int num = 0;
  int max_height = 0;

  for (int s = 0; s < n_stacks; s++) {
    do {
      line++;
      fgets(buf, BUFSIZ, fp);
      iter = buf + strspn(buf, " \t");
    } while (*iter == '#' || *iter == '\0');

    height[s] = strtol(iter, &next, 10);
    if (iter == next) {
      fprintf(stderr, "Failed to read `h[%d]` in line %d: %s\n", s + 1, line,
              buf);
      goto free_resource;
    }
    iter = next;

    for (int t = 1; t <= height[s]; t++) {
      int p = strtol(iter, &next, 10);
      if (iter == next) {
        fprintf(stderr, "Failed to read `p[%d][%d]` in line %d: %s\n", s + 1, t,
                line, buf);
        goto free_resource;
      }
      iter = next;
      blocks[num].s = s;
      blocks[num].t = t;
      blocks[num].p = p;
      num++;
    }
    if (max_height < height[s]) {
      max_height = height[s];
    }
  }

  inst = create_instance(n_stacks, n_tiers);
  inst->n_blocks = n_blocks;
  memcpy(inst->h, height, sizeof(int) * n_stacks);
  for (num = 0; num < n_blocks; num++) {
    int s = blocks[num].s;
    int t = blocks[num].t;
    int p = blocks[num].p;
    inst->p[s][t] = p;
  }

free_resource:
  free(height);
  free(blocks);

return_instance:
  return inst;
}

void print_instance(FILE *fp, instance_t *inst) {
  for (int t = inst->n_tiers; t >= 1; t--) {
    for (int s = 0; s < inst->n_stacks; s++) {
      t > inst->h[s] ? fprintf(fp, "[   ]")
                     : fprintf(fp, "[%3d]", inst->p[s][t]);
    }
    fprintf(fp, "\n");
  }

  for (int s = 0; s < inst->n_stacks; s++) {
    fprintf(fp, "-----");
  }
  fprintf(fp, "\n");

  for (int s = 0; s < inst->n_stacks; s++) {
    fprintf(fp, " %3d ", s);
  }
  fprintf(fp, "\n");
}
