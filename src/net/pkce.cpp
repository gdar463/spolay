/**
 * pkce.cpp
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

#include "pkce.hpp"

#include <httplib.hpp>
#include <openssl/sha.h>

#include "error_macros.hpp"
#include "rand.hpp"

void PKCEClient::cleanup() {
	if (hash) {
		delete[] hash;
	}
	if (current_code) {
		delete[] current_code;
	}
}

std::string PKCEClient::generate_code() {
	if (current_code) {
		delete[] current_code;
	}
	current_code = new unsigned char[PKCE_CODE_LENGTH + 1];
	Random::generate_string(PKCE_CODE_LENGTH, current_code);
	current_code[PKCE_CODE_LENGTH] = '\0';

	if (hash) {
		delete[] hash;
	}
	hash = new unsigned char[SHA256_DIGEST_LENGTH];
	SHA256(current_code, PKCE_CODE_LENGTH, hash);
	return base64_encode(hash, SHA256_DIGEST_LENGTH);
}
const char *PKCEClient::get_current_code() {
	ERR_FAIL_NULL_RET(current_code, nullptr, "PKCE code was not generated first.");
	return (const char *)current_code;
}

std::string PKCEClient::base64_encode(const unsigned char *p_buf, size_t p_len) {
	std::string out = httplib::detail::base64_encode({ (const char *)p_buf, p_len });
	size_t i = 0;
	size_t size = out.size();
	while (i < size) {
		if (out[i] == '=') {
			out.erase(out.begin() + i, out.begin() + i + 1);
			size--;
			continue;
		}
		if (out[i] == '+') {
			out[i] = '-';
		} else if (out[i] == '/') {
			out[i] = '_';
		}
		i++;
	}
	return out;
}
