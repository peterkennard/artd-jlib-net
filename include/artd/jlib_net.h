#ifndef __artd_jlib_net_h
#define __artd_jlib_net_h

#include "artd/jlib_base.h"

// TODO: do we need to handle this for building either static or dll libraries !!

#ifdef BUILDING_artd_jlib_net
	#define ARTD_API_JLIB_NET ARTD_SHARED_LIBRARY_EXPORT
#else
	#define ARTD_API_JLIB_NET ARTD_SHARED_LIBRARY_IMPORT
#endif

#endif // __artd_jlib_net_h
