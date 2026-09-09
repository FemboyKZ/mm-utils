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

	// Scan [base, base+size] for `signature` and report whether a second match exists.
	// `signature` is an IDA byte string, single-spaced, "?" for a wildcard byte, e.g. "48 8B 1D ? ? ? ? 48 85 DB".
	// Double spaces do not parse.
	//
	// Goes through KHook so bytes sitting under an active detour compare as their pre-hook originals.
	// A sibling plugin hooking the same function would otherwise hide the pattern from us.
	// Needs PLUGIN_SAVEVARS() to have run, that is what hands the plugin its KHook interface.
	//
	// A non-unique signature means we can't trust the first hit, so callers should refuse it rather than risk a bad pointer.
	inline void *FindSignatureUnique(void *base, size_t size, const char *signature, bool &outMultiple)
	{
		outMultiple = false;
		if (!base || !signature || size == 0)
		{
			return nullptr;
		}

		void *first = KHook::LookupSignature(base, size, signature);
		if (!first)
		{
			return nullptr;
		}

		// Resume one byte past the hit, so overlapping matches still count as ambiguous.
		size_t consumed = static_cast<size_t>(reinterpret_cast<uintptr_t>(first) - reinterpret_cast<uintptr_t>(base)) + 1;
		if (consumed < size)
		{
			void *next = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(base) + consumed);
			outMultiple = KHook::LookupSignature(next, size - consumed, signature) != nullptr;
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
