#pragma once

/**
 * pkce.hpp
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

#include <string>

#define PKCE_CODE_LENGTH 120

class PKCEClient {
public:
	/// Generates PKCE code and returns the sha256 hash of it
	std::string generate_code();
	/// Returns PKCE code (not its hash)
	const char *get_current_code();

	std::string base64_encode(const unsigned char *p_buf, size_t p_len);

	void cleanup();

private:
	unsigned char *current_code = nullptr;
	unsigned char *hash = nullptr;
};
