#pragma once

#ifndef NO_STANDARD_INCLUDES
#include <string>
#endif

namespace zoo {

using BeforeMovingType = const char *;

BeforeMovingType beforeMoving(const std::string &s) {
	return s.data();
}

auto bufferWasMoved(const std::string &to, BeforeMovingType b) {
	return to.data() == b;
}

}
