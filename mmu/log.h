#ifndef _INCLUDE_MMU_LOG_H_
#define _INCLUDE_MMU_LOG_H_

namespace mmu
{
	// Tag used in bracketed console log lines, e.g. "[CS2Admin] Schema: ...".
	// Assign once at plugin load. Defaults to "mmu".
	extern const char *g_logTag;
} // namespace mmu

#endif // _INCLUDE_MMU_LOG_H_
