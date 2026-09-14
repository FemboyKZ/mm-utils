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

	// Sourced from the CS2Fixes and cs2kz-metamod gamedata files:
	//   https://github.com/Source2ZE/CS2Fixes/blob/main/gamedata/cs2fixes.games.txt
	//   https://github.com/KZGlobalTeam/cs2kz-metamod/blob/master/gamedata/cs2kz-core.games.txt
	namespace gamedata
	{
		// IGameResourceService to CGameEntitySystem* offset. Key "GameEntitySystem".
#ifdef _WIN32
		inline constexpr int kGameEntitySystemOffset = 88;
#else
		inline constexpr int kGameEntitySystemOffset = 80;
#endif

		// CBaseGameSystemFactory::sm_pFirst, RIP-relative mov with disp at +3. Key "IGameSystem_InitAllSystems_pFirst".
#ifdef _WIN32
		inline constexpr const char *kGameSystemFactorySig = "48 8B 1D ? ? ? ? 48 85 DB 0F 84 ? ? ? ? BD";
#else
		inline constexpr const char *kGameSystemFactorySig = "4C 8B 35 ? ? ? ? 4D 85 F6 75 ? E9";
#endif

		// IGameEventManager2 instance, RIP-relative lea with disp at +3. Key "GameEventManager".
#ifdef _WIN32
		// lea rcx, [rip+x] / mov rax, [r8+rdi]
		inline constexpr const char *kGameEventManagerSig = "48 8D 0D ? ? ? ? 49 8B 04 38";
#else
		// lea r12, [rip+x] / xor ebx, ebx / sub rsp, 8
		inline constexpr const char *kGameEventManagerSig = "4C 8D 25 ? ? ? ? 31 DB 48 83 EC 08";
#endif

		// CreateEntityByName, DispatchSpawn and UTIL_Remove. Keys "CreateEntityByName", "DispatchSpawn", "RemoveEntity".
#ifdef _WIN32
		inline constexpr const char *kCreateEntityByNameSig = "48 83 EC 48 C6 44 24 30 00";
		inline constexpr const char *kDispatchSpawnSig = "48 89 5C 24 ? 57 48 83 EC ? 48 8B DA 48 8B F9 48 85 C9 0F 84 ? ? ? ? 48 85 D2";
		inline constexpr const char *kRemoveEntitySig = "48 85 C9 74 ? 48 8B D1 48 8B 0D ? ? ? ?";
#else
		inline constexpr const char *kCreateEntityByNameSig = "48 8D 05 ? ? ? ? 55 48 89 FA";
		inline constexpr const char *kDispatchSpawnSig = "48 85 FF 74 ? 55 48 89 E5 41 55 41 54 49 89 FC";
		inline constexpr const char *kRemoveEntitySig = "48 89 FE 48 85 FF 74 ? 48 8D 05 ? ? ? ? 48";
#endif

		// CCSCustomHudLayout's own setters, from ModSharp's server.games.jsonc:
		//   https://github.com/Kxnrl/modsharp-public/blob/master/.asset/gamedata/server.games.jsonc
		// SetHasClass(this, CUtlString *panel, CUtlString *cls, uint32 status) and
		// SetDialogVariableString(this, CUtlString *panel, CUtlString *name, CUtlString *value) write the global state.
		// SetInputCaptureEnabled(this, int slot, bool) writes one player's state.
#ifdef _WIN32
		inline constexpr const char *kCustomHudSetHasClassSig =
			"48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F0 33 ED 4C 8D 44 24 ? 66 89 6C 24 ? 41 8B F9 48 8B D9";
		inline constexpr const char *kCustomHudSetDialogVariableStringSig =
			"48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 49 8B F0 33 ED 4C 8D 44 24 ? 66 89 6C 24 ? 49 8B F9 48 8B D9";
		inline constexpr const char *kCustomHudSetInputCaptureEnabledSig =
			"40 57 48 83 EC ? 41 0F B6 F8 3B 91 ? ? ? ? 7D ? 48 89 5C 24 ? 48 63 C2 48 69 D8 ? ? ? ? 48 03 99 ? ? ? ?";
#else
		inline constexpr const char *kCustomHudSetHasClassSig =
			"55 48 89 E5 41 57 41 56 41 55 41 89 CD 41 54 49 89 D4 31 D2 53 48 89 FB 48 83 EC ? "
			"66 89 55 ? 48 8D 55 ? E8 ? ? ? ? 84 C0 75 ? 48 83 C4 ? 5B 41 5C 41 5D 41 5E 41 5F 5D C3";
		inline constexpr const char *kCustomHudSetDialogVariableStringSig =
			"55 48 89 E5 41 55 49 89 CD 41 54 49 89 D4 31 D2 53 48 89 FB 48 83 EC ? 66 89 55 ? 48 8D 55 ? E8 ? ? ? ? 84 C0 75 ?";
		inline constexpr const char *kCustomHudSetInputCaptureEnabledSig =
			"3B B7 ? ? ? ? 0F 8D ? ? ? ? 55 48 89 E5 41 55 41 54 41 89 D4 53 48 63 DE 48 69 DB ? ? ? ? 48 83 EC ? 48 03 9F ? ? ? ? 3A 53 ? 75 ?";
#endif
		// CCheckTransmitInfo to the recipient's CPlayerSlot. Key "QuietPlayerSlot".
		inline constexpr int kCheckTransmitPlayerSlotOffset = 576;
	} // namespace gamedata

} // namespace mmu

#endif // _INCLUDE_MMU_GAMEDATA_H_
