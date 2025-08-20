#pragma once

/**
 * @brief Common spdlog configuration and warning suppressions
 *
 * This header provides a centralized way to include spdlog while suppressing
 * known warnings that occur on Windows due to Qt's uint typedef conflicting
 * with spdlog's bundled fmt library.
 */

// Suppress specific warnings on MSVC for spdlog/fmt conflicts
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4459) // declaration hides global declaration
#endif

#include <spdlog/spdlog.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif
