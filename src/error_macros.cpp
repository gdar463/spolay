/**
 * error_macros.cpp
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

#include "error_macros.hpp"

#include <iostream>

void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_msg) {
	std::cerr << p_function << " (at " << p_file << ":" << p_line << "): " << p_error << " " << p_msg << std::endl;
}
void _err_print_error(const char *p_function, const char *p_file, int p_line, std::string p_error, const char *p_msg) {
	std::cerr << p_function << " (at " << p_file << ":" << p_line << "): " << p_error << " " << p_msg << std::endl;
}
void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, std::string p_msg) {
	std::cerr << p_function << " (at " << p_file << ":" << p_line << "): " << p_error << " " << p_msg << std::endl;
}
void _err_print_error(const char *p_function, const char *p_file, int p_line, std::string p_error, std::string p_msg) {
	std::cerr << p_function << " (at " << p_file << ":" << p_line << "): " << p_error << " " << p_msg << std::endl;
}
