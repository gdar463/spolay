#pragma once

/**
 * spotify.hpp
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

#include <httplib.hpp>

#include "net/callback.hpp"
#include "pkce.hpp"

#define SPOTIFY_API_BASE "https://api.spotify.com"
#define SPOTIFY_TOKEN_BASE "https://accounts.spotify.com"
#define SPOTIFY_USER_AGENT APP_NAME "/" APP_VERSION " (by gdar463 dev@gdar463.com)"
#define SPOTIFY_CALLBACK_PORT 18000
#define SPOTIFY_REDIRECT_URI "http://127.0.0.1:{}/callback"
#define SPOTIFY_SCOPES "user-read-playback-state user-modify-playback-state"
#define SPOTIFY_STATE_LENGTH 128

class SpotifyAPI {
public:
	PKCEClient pkce{};
	httplib::Client *client;
	httplib::Client *token_client;
	CallbackServer callback{};

	void setup();
	std::string get_auth_url();
	bool authenticate(std::string p_code, std::string p_state);

	void play();
	void pause();

	void set_client_id(const char *p_id);
	void cleanup();

private:
	const char *client_id = nullptr;
	char *current_state = nullptr;
	std::string current_access_token{ "" };
	uint64_t access_token_expiry = UINT64_MAX;
	std::string current_refresh_token{ "" };

	const char *get_client_id();
};
