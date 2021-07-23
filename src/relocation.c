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

#include "relocation.h"

void print_path(FILE *fp, relocation_t *path, int len) {
  if (len == __INT_MAX__) {
    fprintf(fp, "[?]\n");
  } else {
    fprintf(fp, "[");
    for (int i = 0; i < len; i++) {
      fprintf(fp, "(%d: %d -> %d)%s", path[i].pri, path[i].src, path[i].dst,
              i < len - 1 ? ", " : "");
    }
    fprintf(fp, "]\n");
  }
}
