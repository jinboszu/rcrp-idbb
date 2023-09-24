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

#include "algorithm.h"
#include "lower_bound.h"
#include "timer.h"
#include "upper_bound.h"
#include <limits.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  int lb;
  state_t *state;
} node_t;

typedef struct {
  int dst;
  int q_dst;
  int child_lb;
  state_t *child_state;
} branch_t;

static int compare_branch(const void *a, const void *b) {
  branch_t *x = (branch_t *)a;
  branch_t *y = (branch_t *)b;
  return x->child_lb != y->child_lb ? x->child_lb - y->child_lb
                                    : y->q_dst - x->q_dst;
}

/*
 * Temporary variables
 */
static state_t *root_state;  // for initialization
static state_t *probe_state; // for probing
static int *array_s1;        // for lower bounding
static int *array_s2;        // for lower bounding
static int *array_s3;        // for lower bounding
static int *array_t1;        // for lower bounding
static move_t *path;         // for branch-and-bound
static node_t *hist;         // for branch-and-bound
static state_t *temp_state;  // for branch-and-bound
static branch_t *pool;       // for branch-and-bound

/*
 * Parameters
 */
static int n_stacks;
static int n_tiers;

/*
 * Report
 */
static int best_lb;
static int best_ub;
static move_t *best_sol;
static double start_time;
static double end_time;
static double time_to_best_lb;
static double time_to_best_ub;
static long n_nodes;
static long n_probe;

/*
 * Timer
 */
static long n_timer;
static long timer_cycle;

static void debug_info(char *status) {
  fprintf(stdout,
          "[%s] best_lb = %d @ %.3f / best_ub = %d @ %.3f / time = %.3f / "
          "nodes = %ld / probe = %ld\n",
          status, best_lb, time_to_best_lb - start_time, best_ub,
          time_to_best_ub - start_time, get_time() - start_time, n_nodes,
          n_probe);
  fflush(stdout);
}

/*
 * Branch-and-bound
 */
static bool search(int level, branch_t *branches) {
  n_nodes++;

  /*
   * Check time limit
   */
  if (++n_timer == timer_cycle) {
    n_timer = 0;
    if (get_time() >= end_time) {
      return true;
    }
    debug_info("running");
  }

  /*
   * Current state
   */
  int curr_lb = hist[level].lb;
  state_t *curr_state = hist[level].state;

  /*
   * Source stack
   */
  int sn = curr_state->list[0];
  int pn = curr_state->p[sn][curr_state->h[sn]];

  /*
   * Lower bounding
   */
  int q_max;
  for (int i = n_stacks - 1;; i--) {
    int s = curr_state->list[i];
    if (curr_state->h[s] < n_tiers) {
      q_max = curr_state->q[s][curr_state->h[s]];
      break;
    }
  }
  if (level + curr_lb + (pn > q_max) -
          (curr_lb > curr_state->n_bad && pn > q_max) >
      best_lb) {
    return false;
  }

  /*
   * Prepare branching
   */
  int size = 0;

  /*
   * Enumerate destination stack
   */
  bool first_dn = true;
  bool first_empty = true;
  for (int dn = 0; dn < n_stacks; dn++) {
    /*
     * Check feasibility
     */
    if (dn == sn || curr_state->h[dn] == n_tiers) {
      continue;
    }

    /*
     * Update path when generating branches
     */
    path[level].p = pn;
    path[level].s = sn;
    path[level].d = dn;

    /*
     * Goal test
     */
    int q_dn = curr_state->q[dn][curr_state->h[dn]];
    if (curr_state->n_bad - 1 + (pn > q_dn) == 0) {
      best_ub = level + 1;
      memcpy(best_sol, path, sizeof(move_t) * best_ub);
      time_to_best_ub = get_time();
      debug_info("goal");
      return true;
    }

    /*
     * Check empty stack rule
     */
    if (curr_state->h[dn] == 0) {
      if (first_empty) {
        first_empty = false;
      } else {
        continue; // EA: choose the leftmost empty stack
      }
    }

    /*
     * Check transitive relocation rule
     */
    if (curr_state->last_change_time[dn] <
        curr_state->l[sn][curr_state->h[sn]]) {
      continue;
    }

    /*
     * Lower bounding
     */
    if (level + curr_lb + (pn > q_dn) -
            (curr_lb > curr_state->n_bad && pn > q_dn) >
        best_lb) {
      continue;
    }

    if (first_dn) {
      first_dn = false;
      copy_state_body(hist[level + 1].state, curr_state);
      copy_state_head(temp_state, curr_state);
      reuse_state_body(temp_state, hist[level + 1].state);
      move_out(temp_state, sn, level + 1);
    }
    state_t *child_state = branches[size].child_state;
    copy_state_head(child_state, temp_state);
    reuse_state_body(child_state, hist[level + 1].state);
    move_in(child_state, dn, pn, level + 1);

    /*
     * Retrieve
     */
    bool dominated = false;
    while (is_retrievable(child_state)) {
      int s_min = child_state->list[0];
      int l = child_state->l[s_min][child_state->h[s_min]];

      if (l > 0) {
        for (int d = 0; d < s_min; d++) {
          /*
           * Check retrieval rule
           */
          if (child_state->last_change_time[d] < l &&
              child_state->h[d] <= child_state->h[s_min] - 1) {
            dominated = true;
            break;
          }
        }
        if (dominated) {
          break;
        }
      }

      retrieve(child_state, level + 1);
    }

    if (dominated) {
      continue;
    }

    /*
     * Child lower bound
     */
    int child_lb = lb4(child_state, best_lb - level - child_state->n_bad,
                       array_s1, array_s2, array_s3, array_t1);

    /*
     * Lower bounding
     */
    if (level + 1 + child_lb > best_lb) {
      continue;
    }

    /*
     * Probing
     */
    if (level + 1 + child_lb == best_lb - 1) {
      n_probe++;
      copy_state(probe_state, child_state);
      int new_len = minmax(probe_state, path, level + 1, best_ub - 1);
      if (new_len != INT_MAX) {
        best_ub = new_len;
        memcpy(best_sol, path, sizeof(move_t) * best_ub);
        time_to_best_ub = get_time();
        debug_info("update");
        if (best_lb == best_ub) {
          return true;
        }
      }
    }

    /*
     * Non-dominated branches
     */
    branches[size].dst = dn;
    branches[size].q_dst = q_dn;
    branches[size].child_lb = child_lb;
    size++;
  }

  /*
   * Depth-first search
   */
  if (size > 0) {
    qsort(branches, size, sizeof(branch_t), compare_branch);

    for (int i = 0; i < size; i++) {
      path[level].p = pn;
      path[level].s = sn;
      path[level].d = branches[i].dst;

      hist[level + 1].lb = branches[i].child_lb;
      reuse_state_head(hist[level + 1].state, branches[i].child_state);

      int dn = path[level].d;
      if (hist[level + 1].state->h[dn] == curr_state->h[dn] + 1) {
        update_slot(hist[level + 1].state, dn, hist[level + 1].state->h[dn],
                    path[level].p, level + 1);
      }

      if (search(level + 1, branches + size)) {
        return true;
      }
    }
  }

  return false;
}

report_t *solve(instance_t *inst, int _t) {
  /*
   * Parameters
   */
  n_stacks = inst->n_stacks;
  n_tiers = inst->n_tiers;
  start_time = get_time();
  end_time = start_time + _t;

  /*
   * Root state
   */
  root_state = malloc_state(n_stacks, n_tiers, true, true, true);
  init_state(root_state, inst);
  while (is_retrievable(root_state)) {
    retrieve(root_state, 0);
  }
  if (root_state->n_blocks == 0) {
    free_state(root_state);
    return new_report(0, 0, 0, 0, NULL, 0, 0, 0, 0, 0);
  }

  /*
   * Check if there is a solution
   */
  probe_state = malloc_state(n_stacks, n_tiers, true, true, false);
  copy_state(probe_state, root_state);
  int max_depth = minmax(probe_state, NULL, 0, INT_MAX);
  if (max_depth == INT_MAX) {
    free_state(root_state);
    free_state(probe_state);
    return NULL;
  }

  /*
   * Temporary variables for lower bounding
   */
  array_s1 = malloc(sizeof(int) * n_stacks);
  array_s2 = malloc(sizeof(int) * n_stacks);
  array_s3 = malloc(sizeof(int) * n_stacks);
  array_t1 = malloc(sizeof(int) * n_tiers);

  /*
   * Temporary variables for branch-and-bound
   */
  path = malloc(sizeof(move_t) * max_depth);
  hist = malloc(sizeof(node_t) * (max_depth + 1));
  for (int i = 1; i <= max_depth; i++) {
    hist[i].state = malloc_state(n_stacks, n_tiers, false, true, true);
  }
  temp_state = malloc_state(n_stacks, n_tiers, true, false, true);
  pool = malloc(sizeof(branch_t) * max_depth * (n_stacks - 1));
  for (int i = 0; i < max_depth * (n_stacks - 1); i++) {
    pool[i].child_state = malloc_state(n_stacks, n_tiers, true, false, true);
  }

  /*
   * Root lower bound
   */
  int root_lb =
      lb4(root_state, INT_MAX, array_s1, array_s2, array_s3, array_t1);

  /*
   * Initialize best lower and upper bounds
   */
  best_lb = root_lb;
  time_to_best_lb = start_time;
  best_sol = malloc(sizeof(move_t) * max_depth);
  copy_state(probe_state, root_state);
  best_ub = minmax(probe_state, best_sol, 0, INT_MAX);
  time_to_best_ub = start_time;

  /*
   * Initialize history
   */
  hist[0].state = root_state;
  hist[0].lb = root_lb;

  /*
   * Iterative deepening search
   */
  n_nodes = 0;
  n_probe = 0;
  n_timer = 0;
  timer_cycle = 1000000;

  debug_info("start");
  while (best_lb < best_ub) {
    if (search(0, pool)) {
      break;
    }
    best_lb++;
    time_to_best_lb = get_time();
    debug_info("deepen");
  }
  debug_info("end");

  /*
   * Free temporary variables
   */
  free_state(root_state);
  free_state(probe_state);
  free(array_s1);
  free(array_s2);
  free(array_s3);
  free(array_t1);
  free(path);
  for (int i = 1; i <= max_depth; i++) {
    free_state(hist[i].state);
  }
  free(hist);
  free_state(temp_state);
  for (int i = 0; i < max_depth * (n_stacks - 1); i++) {
    free_state(pool[i].child_state);
  }
  free(pool);

  /*
   * Report
   */
  report_t *report =
      new_report(root_lb, max_depth, best_lb, best_ub, best_sol,
                 time_to_best_lb - start_time, time_to_best_ub - start_time,
                 get_time() - start_time, n_nodes, n_probe);
  free(best_sol);
  return report;
}
