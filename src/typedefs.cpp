/**
 * typedefs.cpp
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

#include "typedefs.hpp"

std::string itos(int64_t p_num, int p_base) {
	bool negative = p_num < 0;

	int64_t n = p_num;
	int digits = negative;
	do {
		n /= p_base;
		digits++;
	} while (n);

	std::string str{};
	// +1 for '\0'
	str.resize(digits + 1);
	char *data = str.data();
	data[digits] = '\0';
	n = p_num;
	do {
		int mod = glm::abs(n % p_base);
		if (mod >= 10) {
			char a = 'A';
			data[--digits] = a + (mod - 10);
		} else {
			data[--digits] = '0' + mod;
		}

		n /= p_base;
	} while (n);

	if (negative) {
		data[0] = '-';
	}

	return str;
}
const char *itoa(int64_t p_num, int p_base) {
	bool sign = p_num >= 0;

	int64_t n = p_num;
	int digits = sign;
	do {
		n /= p_base;
		digits++;
	} while (n);

	// +1 for '\0'
	char *data = new char[digits + 1];
	data[digits] = '\0';
	n = p_num;
	do {
		int mod = glm::abs(n % p_base);
		if (mod >= 10) {
			char a = 'A';
			data[--digits] = a + (mod - 10);
		} else {
			data[--digits] = '0' + mod;
		}

		n /= p_base;
	} while (n);

	if (sign) {
		data[0] = '-';
	}

	return const_cast<const char *>(data);
}
