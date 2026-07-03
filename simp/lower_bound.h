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

#ifndef LOWER_BOUND_H
#define LOWER_BOUND_H

#include "state.h"

/**
 * Compute the value of LB4
 *
 * @param state the state
 * @param h temporary array
 * @param quality temporary array
 * @param priority temporary array
 * @return LB4
 */
int lb4(state_t *state, int *h, int *quality, int *priority);

#endif
