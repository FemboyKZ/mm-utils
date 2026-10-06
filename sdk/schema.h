#ifndef _INCLUDE_MMU_SCHEMA_H_
#define _INCLUDE_MMU_SCHEMA_H_

#include <cstdint>
#include <type_traits>
#include <schemasystem/schemasystem.h>

namespace schema
{
	int16_t GetOffset(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey);

	// GetOffset for a field that may sit at 0, like a plain struct's first member. -1 while unresolved.
	int32_t FindOffset(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey);

	// nullptr unless the field is a CUtlVector-like collection.
	SchemaCollectionManipulatorFn_t GetCollectionManipulator(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey);

	// Grows through the game's manipulator, so new elements are constructed the way the game would.
	// Nothing here notifies clients.
	template<typename T>
	class Collection
	{
	public:
		Collection(void *field, SchemaCollectionManipulatorFn_t manipulator) : m_field(field), m_manipulator(manipulator) {}

		bool IsValid() const
		{
			return m_field && m_manipulator;
		}

		int Count() const
		{
			if (!IsValid())
			{
				return 0;
			}
			return static_cast<int>(reinterpret_cast<intptr_t>(m_manipulator(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_COUNT, m_field, 0, 0)));
		}

		// Unchecked, and every call goes into the game. Hoist Count() out of loops.
		T *At(int index) const
		{
			return static_cast<T *>(m_manipulator(SCHEMA_COLLECTION_MANIPULATOR_ACTION_GET_ELEMENT, m_field, index, 0));
		}

		T *Append() const
		{
			if (!IsValid())
			{
				return nullptr;
			}
			const int count = Count();
			m_manipulator(SCHEMA_COLLECTION_MANIPULATOR_ACTION_SET_COUNT, m_field, count + 1, 0);
			return Count() == count + 1 ? At(count) : nullptr;
		}

	private:
		void *m_field;
		SchemaCollectionManipulatorFn_t m_manipulator;
	};
} // namespace schema

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
		this->NetworkStateChanged(NetworkStateChangedData(static_cast<uint32>(offset))); \
	}

// schema::Collection accessor for a CUtlVector-like field, invalid while unresolved.
#define SCHEMA_COLLECTION(type, fieldName) \
	SCHEMA_FIELD_OFFSET_FN(fieldName) \
	schema::Collection<type> fieldName() \
	{ \
		static constexpr uint32_t fieldHash = FNV1a(#fieldName); \
		const int16_t offset = fieldName##_Offset(); \
		if (offset <= 0) \
		{ \
			return schema::Collection<type>(nullptr, nullptr); \
		} \
		void *field = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(this) + offset); \
		return schema::Collection<type>(field, schema::GetCollectionManipulator(m_className, m_classNameHash, #fieldName, fieldHash)); \
	}

#endif // _INCLUDE_MMU_SCHEMA_H_
