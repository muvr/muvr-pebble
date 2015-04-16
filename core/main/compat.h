#pragma once

// This is a hack for Pebble, which does not have the __unused definition
#ifndef __unused
#define __unused	__attribute__((unused))
#endif

// This is a hack for Pebble, which does not have the ``void exit(int)`` function.
#define EXIT(n) { APP_LOG(APP_LOG_LEVEL_ERROR, "exit(%d)", n); APP_LOG(APP_LOG_LEVEL_ERROR, "%d", (n / n-n)); }
