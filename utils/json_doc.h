#ifndef _INCLUDE_MMU_JSON_DOC_H_
#define _INCLUDE_MMU_JSON_DOC_H_

#include "thirdparty/nlohmann/json.hpp"

#include <cstdint>
#include <string>

namespace mmu
{
	namespace json
	{
		// nlohmann's value() and at() throw on a missing key or another type, and the Linux builds are compiled without exceptions,
		// so a reply shaped otherwise would abort the server. Fields are read through these instead.

		// Null when absent.
		inline const nlohmann::json *Field(const nlohmann::json &obj, const char *key)
		{
			auto it = obj.find(key);
			return it != obj.end() ? &*it : nullptr;
		}

		inline std::string StringField(const nlohmann::json &obj, const char *key)
		{
			const nlohmann::json *v = Field(obj, key);
			return v && v->is_string() ? v->get<std::string>() : std::string();
		}

		inline int IntField(const nlohmann::json &obj, const char *key, int fallback)
		{
			const nlohmann::json *v = Field(obj, key);
			return v && v->is_number_integer() ? v->get<int>() : fallback;
		}

		inline bool BoolField(const nlohmann::json &obj, const char *key, bool fallback)
		{
			const nlohmann::json *v = Field(obj, key);
			return v && v->is_boolean() ? v->get<bool>() : fallback;
		}

		// A whole number or a string as text, "" for anything else. APIs send ids bare or quoted.
		inline std::string ScalarField(const nlohmann::json &obj, const char *key)
		{
			const nlohmann::json *v = Field(obj, key);
			if (!v)
			{
				return std::string();
			}
			if (v->is_number_unsigned())
			{
				return std::to_string(v->get<uint64_t>());
			}
			if (v->is_number_integer())
			{
				return std::to_string(v->get<int64_t>());
			}
			return v->is_string() ? v->get<std::string>() : std::string();
		}
	} // namespace json
} // namespace mmu

#endif // _INCLUDE_MMU_JSON_DOC_H_
