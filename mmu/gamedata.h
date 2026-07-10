#ifndef _INCLUDE_MMU_GAMEDATA_H_
#define _INCLUDE_MMU_GAMEDATA_H_

#include <map>
#include <string>

namespace mmu
{
	// Named offsets loaded from a KV1 gamedata file, resolved per platform:
	//   "GameData" { "Offsets" { "OffsetName" { "windows" "88" "linux" "80" } } }
	class GameData
	{
	public:
		// Load gamedata from a KV1 file. Returns true on success.
		bool Load(const char *path);

		// Get a named offset for the current platform. Returns -1 if not found.
		int GetOffset(const char *name) const;

		void SetOffset(const std::string &name, int value);

	private:
		std::map<std::string, int> m_offsets;
	};
} // namespace mmu

#endif // _INCLUDE_MMU_GAMEDATA_H_
