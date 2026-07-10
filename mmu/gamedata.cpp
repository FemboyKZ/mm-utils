#include "mmu/gamedata.h"
#include "mmu/kv_parser.h"
#include "mmu/str_utils.h"

#include <cstdlib>
#include <fstream>

namespace mmu
{

	static const char *GetPlatformKey()
	{
#ifdef _WIN32
		return "windows";
#else
		return "linux";
#endif
	}

	// Offset subsections contain platform keys, resolved to the current platform during parse.
	static void GameDataHandler(const std::string &section, const std::string &key, const std::string &value, void *userdata)
	{
		auto *gd = static_cast<GameData *>(userdata);
		if (str::ToLower(key) == GetPlatformKey())
		{
			// The section name is the offset name
			gd->SetOffset(section, std::atoi(value.c_str()));
		}
	}

	bool GameData::Load(const char *path)
	{
		std::ifstream file(path);
		if (!file.is_open())
		{
			return false;
		}

		// Expect: "GameData" { "Offsets" { ... } }
		kv::Token root = kv::NextToken(file);
		if (root.kind != kv::TokenType::String)
		{
			return false;
		}

		kv::Token brace = kv::NextToken(file);
		if (brace.kind != kv::TokenType::OpenBrace)
		{
			return false;
		}

		kv::ParseSection(file, root.value, GameDataHandler, this);
		return true;
	}

	int GameData::GetOffset(const char *name) const
	{
		auto it = m_offsets.find(name);
		if (it != m_offsets.end())
		{
			return it->second;
		}
		return -1;
	}

	void GameData::SetOffset(const std::string &name, int value)
	{
		m_offsets[name] = value;
	}

} // namespace mmu
