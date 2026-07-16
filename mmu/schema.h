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

// Resolves and caches this field's offset. Returns 0 while unresolved.
// A 0 offset means the schema wasn't ready.
// Don't cache it, or a call made before the schema system comes up would poison the field.
#define SCHEMA_FIELD_OFFSET_FN(fieldName) \
	static int16_t fieldName##_Offset() \
	{ \
		static constexpr uint32_t fieldHash = FNV1a(#fieldName); \
		static int16_t offset = 0; \
		if (offset <= 0) \
		{ \
			offset = schema::GetOffset(m_className, m_classNameHash, #fieldName, fieldHash); \
		} \
		return offset; \
	}

// Read only schema field accessor.
// Creates an inline method that returns a const reference to the field at the cached offset.
// The reference is const on purpose: an assignment would change the field on the server without ever telling clients.
// Networked fields that need writing use SCHEMA_FIELD_NETWORKED,
// anything else has to go through the game's own setter or a hooked call.
// add_const_t rather than a plain "const type", so that a pointer field ends up a const pointer
// to a mutable object instead of a mutable pointer to a const object.
#define SCHEMA_FIELD(type, fieldName) \
	SCHEMA_FIELD_OFFSET_FN(fieldName) \
	std::add_const_t<type> &fieldName() \
	{ \
		using FieldType = std::add_const_t<type>; \
		const int16_t offset = fieldName##_Offset(); \
		if (offset <= 0) \
		{ \
			/* Hand back a zeroed value rather than reading from offset 0. */ \
			static const FieldType unresolved {}; \
			return unresolved; \
		} \
		return *reinterpret_cast<FieldType *>(reinterpret_cast<uintptr_t>(this) + offset); \
	}

// Read/write accessor for a networked field, adding a Set<fieldName> to SCHEMA_FIELD.
// The setter writes the field and notifies clients via CEntityInstance::NetworkStateChanged.
//
// Only correct when all three hold
//  - the class derives from CEntityInstance (the notify is a virtual call on `this`),
//  - the field is really networked, otherwise the notify is wasted work,
//  - the field sits directly on the class rather than behind a CNetworkVarChainer,
//    which would need the chainer's object and path index instead of `this`.
#define SCHEMA_FIELD_NETWORKED(type, fieldName) \
	SCHEMA_FIELD(type, fieldName) \
	void Set##fieldName(const type &value) \
	{ \
		const int16_t offset = fieldName##_Offset(); \
		if (offset <= 0) \
		{ \
			return; \
		} \
		*reinterpret_cast<type *>(reinterpret_cast<uintptr_t>(this) + offset) = value; \
		this->NetworkStateChanged(NetworkStateChangedData(offset)); \
	}

#endif // _INCLUDE_MMU_SCHEMA_H_
