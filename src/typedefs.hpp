#pragma once

/**
 * typedefs.hpp
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

#include <glm/glm.hpp>
#include <imgui.h>

#include <cstdint>
#include <string>

#define APP_NAME "Spolay"
#define APP_VULKAN_API_VERSION VK_API_VERSION_1_3

struct SDL_Window;
class VulkanEngine;
class ImGuiEngine;

struct AppState {
	float main_scale = 1.0f;
	SDL_Window *window = nullptr;
	VulkanEngine *vk_engine = nullptr;
	ImGuiEngine *imgui_engine = nullptr;
	bool done = false;
	bool show_another_window = false;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
};

//
// The following functions/macros are borrowed from GodotEngine (https://github.com/godotengine/godot)
// under the MIT License (https://github.com/godotengine/godot/blob/5ec4857b340b6284a18b49b2eda462bd250f219a/LICENSE.txt)
//

std::string itos(int64_t p_num, int p_base = 10);
const char *itoa(int64_t p_num, int p_base = 10);

#if defined(__GNUC__)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) x
#define unlikely(x) x
#endif

#ifndef _STR
#define _STR(m_x) #m_x
#define _MKSTR(m_x) _STR(m_x)
#endif

#ifndef _NO_INLINE_
#if defined(__GNUC__)
#define _NO_INLINE_ __attribute__((noinline))
#elif defined(_MSC_VER)
#define _NO_INLINE_ __declspec(noinline)
#else
#define _NO_INLINE_
#endif
#endif
