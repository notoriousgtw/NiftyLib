#pragma once

#include "core/error_base.h"
#include "core/event_base.h"
#include "core/observer.h"
#include "core/util.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <typeindex>
#include <unordered_set>
#include <vector>

#include "core/glfw_common.h"

// #include "core/app.h"
// #include "core/error.h"

namespace nft::Event
{

// #define NFT_ERROR(Err, Msg) ErrorHandler::Error<Err>()
// #define NFT_REGISTER_ERROR(Err) ErrorHandler::Register<Err>()
}	 // namespace nft::Event