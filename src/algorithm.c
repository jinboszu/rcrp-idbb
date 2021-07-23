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
#include "heuristic.h"
#include "lowerbound.h"
#include "timer.h"
#include <stdlib.h>
#include <string.h>

/*
 * Node and branch
 */
typedef struct {
  state_t *state;
  int lb;
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
static relocation_t *path;   // for branch-and-bound
static node_t *hist;         // for branch-and-bound
static state_t *temp_state;  // for branch-and-bound
static branch_t *branches;   // for branch-and-bound

/*
 * Parameters
 */
static int n_stacks;
static int n_tiers;
static double start_time;
static double end_time;

/*
 * Report
 */
static int best_lb;
static int best_ub;
static relocation_t *best_sol;
static long n_nodes;
static long n_timer;

#if defined(VERBOSE) || !defined(NDEBUG)
static void debug_info(char *status) {
  fprintf(stdout,
          "[%s] best_lb = %d / best_ub = %d / time = %.3lf / nodes = %ld\n",
          status, best_lb, best_ub, get_time() - start_time, n_nodes);
  fflush(stdout);
}
#endif

/*
 * Branch-and-bound
 */
static bool search(int level, branch_t *children) {
  /*
   * Check time limit
   */
  if (++n_timer == 1000000) {
    n_timer = 0;
    if (get_time() >= end_time) {
      return true;
    }
#if defined(VERBOSE) || !defined(NDEBUG)
    debug_info("running");
#endif
  }

  /*
   * Current state
   */
  n_nodes++;
  state_t *curr_state = hist[level].state;
  int curr_lb = hist[level].lb;

#ifdef RULES_BY_TANAKA
#else
  /*
   * Prepare EA
   */
  int leftmost = -1;
  for (int s = 0; s < n_stacks; s++) {
    if (curr_state->h[s] == 0) {
      leftmost = s;
      break;
    }
  }
#endif

  /*
   * Prepare branching
   */
  copy_state_body(hist[level + 1].state, curr_state);
  int size = 0;

  /*
   * Source stack
   */
  int sn = curr_state->list[0];
  int pn = curr_state->conf[sn][curr_state->h[sn]].p;
  int lv = curr_state->conf[sn][curr_state->h[sn]].l;

  /*
   * Prepare temporary state
   */
  copy_state_head(temp_state, curr_state);
  reuse_state_body(temp_state, hist[level + 1].state);

  relocate_out(temp_state, sn, level + 1);

  /*
   * Update path when generating branches
   */
  path[level].pri = pn;
  path[level].src = sn;

#ifdef RULES_BY_TANAKA
  /*
   * Enumerate destination stack
   */
  for (int jn = 0; jn < n_stacks; jn++) {
    int dn = curr_state->list[jn];
#else
  /*
   * Enumerate destination stack
   */
  for (int dn = 0; dn < n_stacks; dn++) {
#endif
    /*
     * Check feasibility
     */
    if (dn == sn || curr_state->h[dn] == n_tiers) {
      continue;
    }

#ifdef RULES_BY_TANAKA
    /*
     * Check EA
     *
     * This rule is used in the source code of Tanaka & Mizuno (2018), but not
     * mentioned in their paper.
     */
    if (curr_state->h[dn] == 0 && jn > 0 &&
        curr_state->h[curr_state->list[jn - 1]] == 0) {
      break;
    }
#else
    /*
     * Check EA
     */
    if (curr_state->h[dn] == 0 && dn != leftmost) {
      continue;
    }
#endif

    /*
     * Check TB
     */
    if (curr_state->last_change_time[dn] < lv) {
      continue;
    }

    int q_dn = curr_state->conf[dn][curr_state->h[dn]].q;

    /*
     * Lower bounding
     */
    if (level + curr_lb + (pn > q_dn ? 1 : 0) -
            (curr_lb > curr_state->n_bad && pn > q_dn ? 1 : 0) >
        best_lb) {
      continue;
    }

    state_t *child_state = children[size].child_state;

    copy_state_head(child_state, temp_state);
    reuse_state_body(child_state, hist[level + 1].state);

    relocate_in(child_state, dn, pn, level + 1);

    /*
     * Update path when generating branches
     */
    path[level].dst = dn;

    bool dominated = false;
    while (is_retrievable(child_state)) {
      int s_min = child_state->list[0];
      int l = child_state->conf[s_min][child_state->h[s_min]].l;

#ifdef RULES_BY_TANAKA
      /*
       * Check RB
       */
      if (l > 0) {
        int k = l;
        for (int i = n_stacks - 1; i >= 0; i--) {
          int d = child_state->list[i];
          if ((child_state->h[d] < child_state->h[s_min] - 1 ||
               (child_state->h[d] == child_state->h[s_min] - 1 &&
                child_state->conf[d][child_state->h[d]].q >
                    child_state->conf[s_min][child_state->h[s_min] - 1].q)) &&
              child_state->last_change_time[d] < k) {
            dominated = true;
            break;
          }
        }
        if (dominated) {
          break;
        }
      }
#else
      /*
       * Check RB
       */
      if (l > 0) {
        int k = l;
        for (int d = 0; d < s_min; d++) {
          if (child_state->h[d] <= child_state->h[s_min] - 1 &&
              child_state->last_change_time[d] < k) {
            dominated = true;
            break;
          }
        }
        if (dominated) {
          break;
        }
      }
#endif

      retrieve(child_state, level + 1);
    }

    if (dominated) {
      continue;
    }

    /*
     * Goal test
     */
    if (child_state->n_blocks == 0) {
      best_ub = level + 1;
      memcpy(best_sol, path, sizeof(relocation_t) * best_ub);
#if defined(VERBOSE) || !defined(NDEBUG)
      debug_info("goal");
#endif
      return true;
    }

    /*
     * Child lower bound
     */
    int child_lb =
        internal_lb4(child_state, best_lb - level - child_state->n_bad,
                     array_s1, array_s2, array_t1, array_s3);

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
      copy_state(probe_state, child_state);
      int new_len;
      if (heuristic(probe_state, path, level + 1, best_ub - 1, &new_len)) {
        best_ub = new_len;
        memcpy(best_sol, path, sizeof(relocation_t) * best_ub);
#if defined(VERBOSE) || !defined(NDEBUG)
        debug_info("update");
#endif
        if (best_lb == best_ub) {
          return true;
        }
      }
    }

    children[size].dst = dn;
    children[size].q_dst = q_dn;
    children[size].child_lb = child_lb;
    size++;
  }

  if (size > 0) {
    qsort(children, size, sizeof(branch_t), compare_branch);

    state_t *child_state = hist[level + 1].state;

    for (int i = 0; i < size; i++) {
      path[level].dst = children[i].dst;

      reuse_state_head(child_state, children[i].child_state);
      hist[level + 1].lb = children[i].child_lb;

      int dn = path[level].dst;
      if (child_state->h[dn] == curr_state->h[dn] + 1) {
        set_item(child_state, dn, child_state->h[dn], path[level].pri,
                 level + 1);
      }

      if (search(level + 1, children + size)) {
        return true;
      }
    }
  }

  return false;
}

report_t *solve(instance_t *inst, int _t) {
#if defined(VERBOSE) || !defined(NDEBUG)
  print_instance(stdout, inst);
  fflush(stdout);
#endif

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
  root_state = create_state(n_stacks, n_tiers, true, true);
  init_state(root_state, inst);
  while (is_retrievable(root_state)) {
    retrieve(root_state, 0);
  }
  if (root_state->n_blocks == 0) {
    free_state(root_state, true, true);
    return create_report(0, 0, 0, 0, NULL, 0, 0);
  }

  /*
   * Check if there is a solution
   */
  probe_state = create_state(n_stacks, n_tiers, true, true);
  copy_state(probe_state, root_state);
  int max_depth;
  if (!heuristic(probe_state, NULL, 0, __INT_MAX__, &max_depth)) {
    free_state(root_state, true, true);
    free_state(probe_state, true, true);
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
  path = malloc(sizeof(relocation_t) * max_depth);
  hist = malloc(sizeof(node_t) * (max_depth + 1));
  for (int i = 1; i <= max_depth; i++) {
    hist[i].state = create_state(n_stacks, n_tiers, false, true);
  }
  temp_state = create_state(n_stacks, n_tiers, true, false);
  branches = malloc(sizeof(branch_t) * max_depth * (n_stacks - 1));
  for (int i = 0; i < max_depth * (n_stacks - 1); i++) {
    branches[i].child_state = create_state(n_stacks, n_tiers, true, false);
  }

  /*
   * Root lower bound
   */
  int root_lb = internal_lb4(root_state, __INT_MAX__, array_s1, array_s2,
                             array_t1, array_s3);

  /*
   * Initialize history
   */
  hist[0].state = root_state;
  hist[0].lb = root_lb;

  /*
   * Initialize best lower and upper bounds
   */
  best_lb = root_lb;
  best_sol = malloc(sizeof(relocation_t) * max_depth);
  copy_state(probe_state, root_state);
  heuristic(probe_state, best_sol, 0, __INT_MAX__, &best_ub);

  /*
   * Iterative deepening search
   */
  n_nodes = 0;
  n_timer = 0;
#if defined(VERBOSE) || !defined(NDEBUG)
  debug_info("start");
#endif
  while (best_lb < best_ub) {
    if (search(0, branches)) {
      break;
    }
    best_lb++;
#if defined(VERBOSE) || !defined(NDEBUG)
    debug_info("deepen");
#endif
  }
#if defined(VERBOSE) || !defined(NDEBUG)
  debug_info("end");
  print_path(stdout, best_sol, best_ub);
  fflush(stdout);
#endif

  /*
   * Free temporary variables
   */
  free_state(root_state, true, true);
  free_state(probe_state, true, true);
  free(array_s1);
  free(array_s2);
  free(array_s3);
  free(array_t1);
  free(path);
  for (int i = 1; i <= max_depth; i++) {
    free_state(hist[i].state, false, true);
  }
  free(hist);
  free_state(temp_state, true, false);
  for (int i = 0; i < max_depth * (n_stacks - 1); i++) {
    free_state(branches[i].child_state, true, false);
  }
  free(branches);

  /*
   * Report
   */
  return create_report(root_lb, max_depth, best_lb, best_ub, best_sol, n_nodes,
                       get_time() - start_time);
}
