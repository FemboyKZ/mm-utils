#ifndef _INCLUDE_MMU_SIGSCAN_H_
#define _INCLUDE_MMU_SIGSCAN_H_

#include <khook.hpp>

#include <cstddef>
#include <cstdint>

namespace sig
{
	// Locate the loaded server module's mapped memory range by walking out from a known pointer that lives inside it (e.g. g_pServerGameDLL).
	// Returns false if the range can't be determined. Implemented per-platform in sigscan.cpp.
	bool GetModuleRange(const void *knownAddress, void *&outBase, size_t &outSize);

	// Find `signature` in [base, base+size], setting outMultiple on a second match. Callers should refuse those.
	// IDA format, single-spaced, "?" wildcard, e.g. "48 8B 1D ? ? ? ?".
	// Uses KHook so bytes under another plugin's detour compare as originals. Needs PLUGIN_SAVEVARS().
	inline void *FindSignatureUnique(void *base, size_t size, const char *signature, bool &outMultiple)
	{
		outMultiple = false;
		if (!base || !signature || !*signature)
		{
			return nullptr;
		}

		// KHook reads a whole signature from each start offset without checking `size`,
		// so only offer starts where a full match still fits.
		size_t sigLen = 1;
		for (const char *p = signature; *p; p++)
		{
			if (*p == ' ')
			{
				sigLen++;
			}
		}
		if (size < sigLen)
		{
			return nullptr;
		}
		size_t window = size - sigLen + 1;

		void *first = KHook::LookupSignature(base, window, signature);
		if (!first)
		{
			return nullptr;
		}

		// One past the hit, so overlapping matches count too.
		size_t consumed = static_cast<size_t>(reinterpret_cast<uintptr_t>(first) - reinterpret_cast<uintptr_t>(base)) + 1;
		if (consumed < window)
		{
			void *next = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(base) + consumed);
			outMultiple = KHook::LookupSignature(next, window - consumed, signature) != nullptr;
		}
		return first;
	}

	// Locate a named section of the module containing `knownAddress`, e.g. ".rdata" on Windows, ".rodata" on Linux.
	bool GetModuleSection(const void *knownAddress, const char *name, void *&outBase, size_t &outSize);

	// Resolve a class vtable by RTTI name, e.g. "CServerSideClient". `knownAddress` is any pointer inside the module that defines it.
	// Returns the first virtual function slot, or null.
	// Only finds primary vtables (base offset 0) of classes the compiler emitted RTTI for.
	void *FindVirtualTable(const void *knownAddress, const char *className);

	// Resolve a RIP-relative MOV/LEA whose displacement begins 3 bytes into the instruction (REX + opcode + ModRM), e.g. `48 8D 0D <disp32>`.
	// Returns the address the instruction computes: nextInsn + signed disp32.
	inline void *ResolveRipRelative(void *instruction)
	{
		if (!instruction)
		{
			return nullptr;
		}
		uint8_t *p = static_cast<uint8_t *>(instruction) + 3;
		int32_t disp = *reinterpret_cast<int32_t *>(p);
		p += 4; // advance to the next instruction (RIP base)
		return p + disp;
	}
} // namespace sig

#endif // _INCLUDE_MMU_SIGSCAN_H_
