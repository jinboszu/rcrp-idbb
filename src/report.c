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

#include "report.h"
#include <stdlib.h>

report_t *create_report(int init_lb, int init_ub, int best_lb, int best_ub,
                        relocation_t *best_sol, long n_nodes,
                        double time_used) {
  report_t *report = malloc(sizeof(report_t));
  report->init_lb = init_lb;
  report->init_ub = init_ub;
  report->best_lb = best_lb;
  report->best_ub = best_ub;
  report->best_sol = best_sol;
  report->n_nodes = n_nodes;
  report->time_used = time_used;
  return report;
}

void free_report(report_t *report) {
  if (report->best_sol != NULL) {
    free(report->best_sol);
  }
  free(report);
}
