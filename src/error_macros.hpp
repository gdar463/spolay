#pragma once

/**
 * error_macros.hpp
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

#include "typedefs.hpp" // IWYU pragma: export

_NO_INLINE_ void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, const char *p_msg);
_NO_INLINE_ void _err_print_error(const char *p_function, const char *p_file, int p_line, std::string p_error, const char *p_msg);
_NO_INLINE_ void _err_print_error(const char *p_function, const char *p_file, int p_line, const char *p_error, std::string p_msg);
_NO_INLINE_ void _err_print_error(const char *p_function, const char *p_file, int p_line, std::string p_error, std::string p_msg);

#pragma region "SDL error_macros"
#define CLEANUP_SDL SDL_AppQuit

#define ERR_FAIL_NULL_SDL(m_param, m_msg) ERR_FAIL_NULL_PRE(m_param, CLEANUP_SDL(), m_msg)
#define ERR_FAIL_NULL_RET_SDL(m_param, m_retval, m_msg) ERR_FAIL_NULL_PRERET(m_param, m_retval, CLEANUP_SDL(), m_msg)

#define ERR_FAIL_COND_SDL(m_cond, m_msg) ERR_FAIL_COND_PRE(m_cond, CLEANUP_SDL(), m_msg)
#define ERR_FAIL_COND_RET_SDL(m_cond, m_retval, m_msg) ERR_FAIL_COND_PRERET(m_cond, m_retval, CLEANUP_SDL(), m_msg)
#define ERR_FAIL_COND_SILENT_SDL(m_cond) ERR_FAIL_COND_SILENT_PRE(m_cond, CLEANUP_SDL())
#define ERR_FAIL_COND_SILENT_RET_SDL(m_cond, m_retval) ERR_FAIL_COND_SILENT_PRERET(m_cond, m_retval, CLEANUP_SDL())
#pragma endregion "SDL error_macros"

#pragma region "VK error_macros"
/*
 * Ensures `m_vk_res` is VK_SUCCESS.
 * If `m_vk_res` is not VK_SUCCESS but not an error, prints message.
 * If `m_vk_res` is an error, prints `m_msg`, and the current function returns.
 */
#define ERR_FAIL_VK(m_vk_res, m_msg) \
	if (VkResult err = m_vk_res; unlikely(err != VK_SUCCESS)) { \
		if (err < 0) { \
			_err_print_error(__FUNCTION__, __FILE__, __LINE__, std::string("Call \"" _STR(m_vk_res) "\" returned ").append(itos(err)).append("."), m_msg); \
			return; \
		} else { \
			_err_print_error(__FUNCTION__, __FILE__, __LINE__, std::string("Call \"" _STR(m_vk_res) "\" returned ").append(itos(err)).append("."), ""); \
		} \
	} else \
		((void)0)

/*
 * Ensures `m_vk_res` is VK_SUCCESS.
 * If `m_vk_res` is not VK_SUCCESS but not an error, prints message.
 * If `m_vk_res` is an error, prints `m_msg`, and the current function returns `m_retval`.
 */
#define ERR_FAIL_VK_RET(m_vk_res, m_retval, m_msg) \
	if (VkResult err = m_vk_res; unlikely(err != VK_SUCCESS)) { \
		if (err < 0) { \
			_err_print_error(__FUNCTION__, __FILE__, __LINE__, std::string("Call \"" _STR(m_vk_res) "\" returned ").append(itos(err)).append("."), m_msg); \
			return m_retval; \
		} else { \
			_err_print_error(__FUNCTION__, __FILE__, __LINE__, std::string("Call \"" _STR(m_vk_res) "\" returned ").append(itos(err)).append("."), ""); \
		} \
	} else \
		((void)0)
#pragma endregion "VK error_macros"

/**
 * Ensures `m_cond` is true.
 * If `m_cond` is false, the current function returns.
 */
#define ERR_FAIL_COND_SILENT(m_cond) \
	if (unlikely(m_cond)) { \
		return; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is true.
 * If `m_cond` is false, the current function returns `m_retval`.
 */
#define ERR_FAIL_COND_SILENT_RET(m_cond, m_retval) \
	if (unlikely(m_cond)) { \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is true.
 * If `m_cond` is false, runs `m_on_fail`, and the current function returns.
 */
#define ERR_FAIL_COND_SILENT_PRE(m_cond, m_on_fail) \
	if (unlikely(m_cond)) { \
		m_on_fail; \
		return; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is true.
 * If `m_cond` is false, runs `m_on_fail`, the current function returns `m_retval`.
 */
#define ERR_FAIL_COND_SILENT_PRERET(m_cond, m_retval, m_on_fail) \
	if (unlikely(m_cond)) { \
		m_on_fail; \
		return m_retval; \
	} else \
		((void)0)

/**
 * Prints `m_msg` and the current functions returns.
 */
#define ERR_FAIL_MSG(m_msg) \
	_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Generic error.", m_msg); \
	return

/**
 * Prints `m_msg` and the current functions returns `m_retval`.
 */
#define ERR_FAIL_MSG_RET(m_retval, m_msg) \
	_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Generic error.", m_msg); \
	return m_retval

/**
 * Ensures a pointer `m_param` is not null.
 * If `m_cond` is true, prints `m_msg`, runs `m_on_fail`, and the current function returns.
 */
#define ERR_FAIL_NULL_PRE(m_param, m_on_fail, m_msg) \
	if (unlikely(m_param == nullptr)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
		m_on_fail; \
		return; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg`, runs `m_on_fail`, and the current function returns `m_retval`.
 */
#define ERR_FAIL_NULL_PRERET(m_param, m_retval, m_on_fail, m_msg) \
	if (unlikely(m_param == nullptr)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
		m_on_fail; \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If it is null, prints `m_msg`, runs `m_on_fail`, and the current function returns.
 */
#define ERR_FAIL_COND_PRE(m_cond, m_on_fail, m_msg) \
	if (unlikely(m_cond)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true.", m_msg); \
		m_on_fail; \
		return; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg`, runs `m_on_fail`, and the current function returns `m_retval`.
 */
#define ERR_FAIL_COND_PRERET(m_cond, m_retval, m_on_fail, m_msg) \
	if (unlikely(m_cond)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true.", m_msg); \
		m_on_fail; \
		return m_retval; \
	} else \
		((void)0)

//
// The following macros are borrowed from Godot (https://github.com/godotengine/godot)
// under the MIT License (https://github.com/godotengine/godot/blob/5ec4857b340b6284a18b49b2eda462bd250f219a/LICENSE.txt)
//

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns.
 *
 * If on fail something should run, use ERR_FAIL_NULL_PRE instead.
 */
#define ERR_FAIL_NULL(m_param, m_msg) \
	if (unlikely(m_param == nullptr)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
		return; \
	} else \
		((void)0)

/**
 * Ensures a pointer `m_param` is not null.
 * If it is null, prints `m_msg` and the current function returns `m_retval`.
 *
 * If on fail something should run, use ERR_FAIL_NULL_PRERET instead.
 */
#define ERR_FAIL_NULL_RET(m_param, m_retval, m_msg) \
	if (unlikely(m_param == nullptr)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Parameter \"" _STR(m_param) "\" is null.", m_msg); \
		return m_retval; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and the current function returns.
 *
 * If on fail something should run, use ERR_FAIL_COND_PRE instead.
 */
#define ERR_FAIL_COND(m_cond, m_msg) \
	if (unlikely(m_cond)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true.", m_msg); \
		return; \
	} else \
		((void)0)

/**
 * Ensures `m_cond` is false.
 * If `m_cond` is true, prints `m_msg` and current function returns `m_retval`.
 *
 * If on fail something should run, use ERR_FAIL_COND_PRERET instead.
 */
#define ERR_FAIL_COND_RET(m_cond, m_retval, m_msg) \
	if (unlikely(m_cond)) { \
		_err_print_error(__FUNCTION__, __FILE__, __LINE__, "Condition \"" _STR(m_cond) "\" is true.", m_msg); \
		return m_retval; \
	} else \
		((void)0)
