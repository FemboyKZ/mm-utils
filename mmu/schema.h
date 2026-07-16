#ifndef _INCLUDE_MMU_SCHEMA_H_
#define _INCLUDE_MMU_SCHEMA_H_

#include <cstdint>
#include <type_traits>
#include <schemasystem/schemasystem.h>

namespace schema
{
	int16_t GetOffset(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey);
}

// FNV1a hash at compile time
constexpr uint32_t FNV1a(const char *str)
{
	uint32_t hash = 0x811C9DC5u;
	while (*str)
	{
		hash ^= static_cast<uint32_t>(*str++);
		hash *= 0x01000193u;
	}
	return hash;
}

// Declares the schema class metadata for this wrapper.
// Must appear at the top of each entity wrapper class body.
#define DECLARE_SCHEMA_CLASS(className) \
	static constexpr const char *m_className = #className; \
	static constexpr uint32_t m_classNameHash = FNV1a(#className);

// Read only schema field accessor.
// Creates an inline method that returns a const reference to the field at the cached offset.
// The reference is const on purpose: this header has no NetworkStateChanged plumbing,
// so an assignment would change the field on the server without ever telling clients.
// Fields that need writing have to go through the game's own setter or a hooked call.
// add_const_t rather than a plain "const type", so that a pointer field ends up a const
// pointer to a mutable object instead of a mutable pointer to a const object.
#define SCHEMA_FIELD(type, fieldName) \
	std::add_const_t<type> &fieldName() \
	{ \
		using FieldType = std::add_const_t<type>; \
		static constexpr uint32_t fieldHash = FNV1a(#fieldName); \
		/* A 0 offset means the schema wasn't ready. */ \
		/* Don't cache it, or a call made before the schema system comes up would poison the field */ \
		static int16_t offset = 0; \
		if (offset <= 0) \
		{ \
			offset = schema::GetOffset(m_className, m_classNameHash, #fieldName, fieldHash); \
			if (offset <= 0) \
			{ \
				/* Hand back a zeroed value rather than reading from offset 0. */ \
				static const FieldType unresolved {}; \
				return unresolved; \
			} \
		} \
		return *reinterpret_cast<FieldType *>(reinterpret_cast<uintptr_t>(this) + offset); \
	}

#endif // _INCLUDE_MMU_SCHEMA_H_
