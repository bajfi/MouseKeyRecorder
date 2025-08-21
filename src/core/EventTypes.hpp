// Copyright (c) 2025 JackLee
//
// This software is released under the MIT License.
// https://opensource.org/licenses/MIT

#pragma once

#include "Event.hpp"
#include <vector>
#include <memory>

namespace MouseRecorder::Core
{
/**
 * @brief Type alias for vector of unique_ptr<Event>
 *
 * This alias is used to avoid MOC compilation issues with Qt when
 * complex template types are used in Q_OBJECT classes.
 * The Qt Meta-Object Compiler (MOC) has issues with complex template
 * types containing unique_ptr because it tries to register them for
 * the meta-type system, which requires copyable types.
 */
using EventVector = std::vector<std::unique_ptr<Event>>;

} // namespace MouseRecorder::Core
