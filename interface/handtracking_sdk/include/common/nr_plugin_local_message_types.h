#pragma once

#include "nr_plugin_types.h"

#ifdef NRAPP

#include "nr_plugin_local_message_types.inc"

#else

NR_PLUGIN_ENUM(NRConnectionState) {
    NR_CONNECTION_UNKNOWN = 0,
    NR_CONNECTION_NEW,
    NR_CONNECTION_REMOVE,
};


#endif // NRAPP