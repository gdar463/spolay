/**
 * spotify.cpp
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

#include "spotify.hpp"

#include <nlohmann/json.hpp>

#include "error_macros.hpp"
#include "rand.hpp"

#if __has_include("env.hpp")
#include "env.hpp"
#endif

void SpotifyAPI::cleanup() {
	if (current_state) {
		delete[] current_state;
	}
	if (client_id) {
		delete[] client_id;
	}
	if (token_client) {
		delete token_client;
	}
	if (client) {
		delete client;
	}
	callback.cleanup();
	pkce.cleanup();
}

void SpotifyAPI::setup() {
	client = new httplib::Client(SPOTIFY_API_BASE);
	client->set_error_logger([](const httplib::Error &p_err, const httplib::Request *p_req) {
		std::cerr << "[SpotifyAPI] Httplib Error: " << p_err;
		if (p_req) {
			std::cerr << " on request " << p_req->method << " " << p_req->path;
		}
		std::cerr << std::endl;
	});
	client->set_default_headers({ { "User-Agent", SPOTIFY_USER_AGENT } });
	token_client = new httplib::Client(SPOTIFY_TOKEN_BASE);
	token_client->set_error_logger([](const httplib::Error &p_err, const httplib::Request *p_req) {
		std::cerr << "[SpotifyAPI] Token - Httplib Error: " << p_err;
		if (p_req) {
			std::cerr << " on request " << p_req->method << " " << p_req->path;
		}
		std::cerr << std::endl;
	});
	token_client->set_default_headers({ { "User-Agent", SPOTIFY_USER_AGENT } });
	token_client->enable_server_certificate_verification(false);
	callback.setup(this);
}
std::string SpotifyAPI::get_auth_url() {
	std::string code = pkce.generate_code();
	if (current_state) {
		delete[] current_state;
	}
	current_state = new char[SPOTIFY_STATE_LENGTH + 1];
	Random::generate_string(SPOTIFY_STATE_LENGTH, current_state);
	current_state[SPOTIFY_STATE_LENGTH] = '\0';

	callback.start(SPOTIFY_CALLBACK_PORT);
	return std::format(SPOTIFY_TOKEN_BASE "/authorize?client_id={}"
										  "&response_type=code"
										  "&redirect_uri={}"
										  "&scope={}"
										  "&code_challenge_method=S256"
										  "&code_challenge={}"
										  "&state={}",
			get_client_id(), httplib::encode_query_component(std::format(SPOTIFY_REDIRECT_URI, SPOTIFY_CALLBACK_PORT)), SPOTIFY_SCOPES, code, current_state);
}
bool SpotifyAPI::authenticate(std::string p_code, std::string p_state) {
	for (int i = 0; i < SPOTIFY_STATE_LENGTH; i++) {
		ERR_FAIL_COND_RET(p_state[i] != current_state[i], false, "States don't match. failed on char " + itos(i) + " with p_state='" + p_state[i] + "' and current_state='" + current_state[i] + "'.");
	}
	httplib::Params params{
		{ "client_id", get_client_id() },
		{ "grant_type", "authorization_code" },
		{ "code", p_code },
		{ "redirect_uri", std::format(SPOTIFY_REDIRECT_URI, SPOTIFY_CALLBACK_PORT) },
		{ "code_verifier", pkce.get_current_code() }
	};
	httplib::Result res = token_client->Post("/api/token", params);
	if (!res) {
		httplib::Error err = res.error();
		if (err == httplib::Error::SSLConnection ||
				err == httplib::Error::SSLServerVerification) {
			std::cerr << "ssl_error: " << res.ssl_error() << std::endl;
			std::cerr << "ssl_backend_error: " << res.ssl_backend_error() << std::endl;
			if (res.ssl_backend_error() != 0) {
				char buf[256];
				ERR_error_string_n(res.ssl_backend_error(), buf, sizeof(buf));
				std::cerr << "openssl: " << buf << std::endl;
			}
		}
		ERR_FAIL_MSG_RET(false, "Failed to get access token.");
	}
	if (res->status != 200) {
		std::cerr << res->body << std::endl;
		ERR_FAIL_MSG_RET(false, "Access token response returned " + itos(res->status) + ".");
	}
	nlohmann::json json = nlohmann::json::parse(res->body);
	current_access_token = json.at("access_token");
	current_refresh_token = json.at("refresh_token");
	access_token_expiry = SDL_GetTicks() + json.at("expires_in").get<int>() * 1000;
	ERR_FAIL_COND_RET(current_access_token.empty(), false, "Access token is empty.");
	callback.stop();
	return true;
}
void SpotifyAPI::play() {
	if (current_access_token == "") {
		ERR_FAIL_MSG("Unauthenticated.");
	}
	if (access_token_expiry < SDL_GetTicks()) {
		ERR_FAIL_MSG("Not implemented.");
	}
	httplib::Headers headers{
		{ "Authorization", "Bearer " + current_access_token }
	};
	httplib::Result res = client->Put("/v1/me/player/play", headers);
	ERR_FAIL_COND(!res, "Failed to play playback.");
	if (res->status != 200) {
		std::cerr << res->body << std::endl;
		ERR_FAIL_MSG("Play returned " + itos(res->status) + ".");
	}
}
void SpotifyAPI::pause() {
	if (current_access_token == "") {
		ERR_FAIL_MSG("Unauthenticated.");
	}
	if (access_token_expiry < SDL_GetTicks()) {
		ERR_FAIL_MSG("Not implemented.");
	}
	httplib::Headers headers{
		{ "Authorization", "Bearer " + current_access_token }
	};
	httplib::Result res = client->Put("/v1/me/player/pause", headers);
	ERR_FAIL_COND(!res, "Failed to pause playback.");
	if (res->status != 200) {
		std::cerr << res->body << std::endl;
		ERR_FAIL_MSG("Pause returned " + itos(res->status) + ".");
	}
}

void SpotifyAPI::set_client_id(const char *p_id) {
	if (!p_id) {
		client_id = nullptr;
	} else {
		client_id = strdup(p_id);
	}
}

const char *SpotifyAPI::get_client_id() {
	if (client_id) {
		return client_id;
	}
#if defined(ENV_HPP)
	return SPOTIFY_CLIENT_ID;
#else
	ERR_FAIL_MSG_RET(nullptr, "Spotify Client ID has not been set.");
#endif
}
