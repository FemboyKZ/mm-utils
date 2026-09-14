#include "mmu/schema.h"
#include "mmu/log.h"

#include <interfaces/interfaces.h>

#include <map>

namespace
{
	struct FieldInfo
	{
		int32_t offset;
		SchemaCollectionManipulatorFn_t manipulator;
	};
} // namespace

// Per class cache: fieldNameHash -> field
static std::map<uint32_t, std::map<uint32_t, FieldInfo>> g_schemaCache;

// Returns false when the schema system isn't ready yet.
static bool InitSchemaFieldsForClass(const char *className, uint32_t classKey)
{
	// Null until the plugin acquires SCHEMASYSTEM_INTERFACE_VERSION in Load().
	// Nothing is cached here, so a later call still resolves once it is set.
	if (!g_pSchemaSystem)
	{
		MMU_LOG_WARN("Schema: schema system unavailable, is g_pSchemaSystem acquired in Load()?\n");
		return false;
	}

	CSchemaSystemTypeScope *pScope = g_pSchemaSystem->FindTypeScopeForModule(
#ifdef _WIN32
		"server.dll"
#else
		"libserver.so"
#endif
	);

	if (!pScope)
	{
		MMU_LOG_WARN("Schema: Could not find server type scope\n");
		return false;
	}

	SchemaClassInfoData_t *pClassInfo = pScope->FindDeclaredClass(className).Get();
	if (!pClassInfo)
	{
		// Mark as probed with an empty entry so a missing class doesn't re-scan every call.
		g_schemaCache[classKey];
		MMU_LOG_WARN("Schema: Could not find class '%s'\n", className);
		return false;
	}

	// Creating the entry (even with zero fields) marks this class as probed,
	// so a fully-resolved-but-field-absent lookup won't re-scan every call.
	auto &classMap = g_schemaCache[classKey];

	for (int i = 0; i < pClassInfo->m_nFieldCount; i++)
	{
		auto &field = pClassInfo->m_pFields[i];
		SchemaCollectionManipulatorFn_t manipulator = nullptr;
		CSchemaType *type = field.m_pType;
		if (type && type->m_eTypeCategory == SCHEMA_TYPE_ATOMIC && type->m_eAtomicCategory == SCHEMA_ATOMIC_COLLECTION_OF_T)
		{
			manipulator = static_cast<CSchemaType_Atomic_CollectionOfT *>(type)->m_pfnManipulator;
		}
		classMap[FNV1a(field.m_pszName)] = {field.m_nSingleInheritanceOffset, manipulator};
	}
	return true;
}

static const FieldInfo *FindField(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey)
{
	auto classIt = g_schemaCache.find(classKey);
	if (classIt == g_schemaCache.end())
	{
		InitSchemaFieldsForClass(className, classKey);
		classIt = g_schemaCache.find(classKey);
	}

	if (classIt != g_schemaCache.end())
	{
		auto fieldIt = classIt->second.find(fieldKey);
		if (fieldIt != classIt->second.end())
		{
			return &fieldIt->second;
		}
	}

	MMU_LOG_WARN("Schema: Could not find offset for %s::%s\n", className, fieldName);
	return nullptr;
}

int16_t schema::GetOffset(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey)
{
	const FieldInfo *field = FindField(className, classKey, fieldName, fieldKey);
	return field ? static_cast<int16_t>(field->offset) : 0;
}

int32_t schema::FindOffset(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey)
{
	const FieldInfo *field = FindField(className, classKey, fieldName, fieldKey);
	return field ? field->offset : -1;
}

SchemaCollectionManipulatorFn_t schema::GetCollectionManipulator(const char *className, uint32_t classKey, const char *fieldName, uint32_t fieldKey)
{
	const FieldInfo *field = FindField(className, classKey, fieldName, fieldKey);
	return field ? field->manipulator : nullptr;
}
