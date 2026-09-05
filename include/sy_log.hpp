#pragma once

#include <OS/OSError.h>

/**
 * @file sy_log.hpp
 * @brief Compile-time gated logging for Syringe core.
 *
 * Logging is routed through SY_LOG so it can be stripped entirely from release
 * builds. When SY_DEBUG is defined (the default build), SY_LOG forwards to
 * OSReport. In release builds (make release) SY_DEBUG is undefined and every
 * SY_LOG call - including its format string and argument marshalling - is
 * compiled out, reclaiming both .rodata (format strings) and .text (call-site
 * code).
 *
 * OSReport itself links to the base game and costs nothing; only the call-site
 * code and format strings live in sy_core, which is what this removes.
 */

#ifdef SY_DEBUG
#define SY_LOG(...) OSReport(__VA_ARGS__)
#else
#define SY_LOG(...) ((void)0)
#endif
