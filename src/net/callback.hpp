#pragma once

/**
 * callback.hpp
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

class SpotifyAPI;

class CallbackServer {
public:
	httplib::Server *server;
	static SpotifyAPI *spotify;
	std::thread *listen_thread;

	void setup(SpotifyAPI *p_spotify);
	void start(int p_port);
	void stop();
	static void callback(const httplib::Request &p_req, httplib::Response &r_res);

	void cleanup();

private:
	static void real_start(httplib::Server *p_server, int p_port);
};
