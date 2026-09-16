#pragma once

/**
 * rand.hpp
 *
 * Copyright (C) 2026 gdar463 <dev@gdar463.com>
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General
 * Public License along with this program. If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <cstdlib>

class Random {
public:
	/// Ensure r_out has a length equal to p_size + 1
	template <typename T>
	static inline T *generate_string(int p_size, T *r_out) {
		for (int i = 0; i < p_size; i++) {
			r_out[i] = characters[rand() % 62];
		}
	}

private:
	constexpr static const char *const characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
};
