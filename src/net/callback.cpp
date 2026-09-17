/**
 * callback.cpp
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

#include "callback.hpp"

#include "spotify.hpp"

#define CALLBACK_AUTH_ERROR_HTML "<html><body><p>Failed to link with Spotify. Received error: {}.</p></body></html>"
#define CALLBACK_TOKEN_ERROR_HTML "<html><body><p>Failed to get tokens from Spotify.</p></body></html>"
#define CALLBACK_AUTH_SUCCESS_HTML "<html><body><p>Linked correctly with Spotify. You can close this tab and return to the app.</p></body></html>"

SpotifyAPI *CallbackServer::spotify = nullptr;

void CallbackServer::cleanup() {
	if (listen_thread) {
		if (listen_thread->joinable()) {
			stop();
			listen_thread->join();
		}
		delete listen_thread;
	}
	if (server) {
		stop();
		delete server;
	}
}

void CallbackServer::setup(SpotifyAPI *p_spotify) {
	spotify = p_spotify;
	server = new httplib::Server();
	server->Get("/callback", callback);
}
void CallbackServer::real_start(httplib::Server *p_server, int p_port) {
	p_server->listen("127.0.0.1", p_port);
}
void CallbackServer::start(int p_port) {
	listen_thread = new std::thread(real_start, server, p_port);
}
void CallbackServer::stop() {
	if (server->is_running()) {
		server->stop();
	}
}

void CallbackServer::callback(const httplib::Request &p_req, httplib::Response &r_res) {
	if (p_req.has_param("error")) {
		r_res.set_content(std::format(CALLBACK_AUTH_ERROR_HTML, p_req.get_param_value("error")), "text/html");
	} else {
		if (!spotify->authenticate(p_req.get_param_value("code"), p_req.get_param_value("state"))) {
			r_res.set_content(CALLBACK_TOKEN_ERROR_HTML, "text/html");
		} else {
			r_res.set_content(CALLBACK_AUTH_SUCCESS_HTML, "text/html");
		}
	}
}
