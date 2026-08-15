#include "mmu/workshop.h"
#include "mmu/gamesystem.h"
#include "mmu/log.h"

#include <filesystem.h>
#include <interfaces/interfaces.h>
#include <KeyValues.h>
#include <tier1/utlmap.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
	template<typename T>
	using WorkshopTree = CUtlOrderedMap<uint64_t, T, bool (*)(const uint64_t &, const uint64_t &), int>;

	class CDedicatedServerWorkshopManager
	{
	public:
		// vtable, game system name, UGC path resolver, SteamWorks callbacks
		uint8_t pad0[0x70];                           // 0x00
		WorkshopTree<void *> m_requestedMaps;         // 0x70
		WorkshopTree<void *> m_mapLoadedWorkshopMaps; // 0x98
		WorkshopTree<bool> m_mapStatus;               // 0xC0
		uint64_t m_nRequestedSharedFileId;            // 0xE8
		uint64_t m_nRequestedCollectionSharedFileId;  // 0xF0
		bool m_bInitialized;                          // 0xF8
	};

#ifdef _WIN32
	static_assert(sizeof(WorkshopTree<void *>) == 0x28, "CUtlOrderedMap layout drifted from the engine's");
	static_assert(offsetof(CDedicatedServerWorkshopManager, m_mapLoadedWorkshopMaps) == 0x98, "workshop manager layout drifted");
	static_assert(sizeof(CDedicatedServerWorkshopManager) == 0x100, "workshop manager layout drifted");
#endif

	CDedicatedServerWorkshopManager *g_pWorkshopMgr = nullptr;

	std::string WorkshopRootAbs()
	{
		std::error_code ec;
		fs::path abs = fs::absolute(fs::path("steamapps") / "workshop", ec);
		if (ec)
		{
			return std::string("steamapps/workshop");
		}
		return abs.string();
	}

	// True if `steamapps/workshop/content/730/<id>/` exists AND contains at least one .vpk file.
	bool WorkshopFolderHasVPK(const std::string &workshopId)
	{
		if (workshopId.empty())
		{
			return false;
		}

		fs::path folder = fs::path(WorkshopRootAbs()) / "content" / "730" / workshopId;

		std::error_code ec;
		if (!fs::is_directory(folder, ec))
		{
			return false;
		}

		fs::directory_iterator it(folder, ec);
		if (ec)
		{
			return false;
		}

		for (const auto &entry : it)
		{
			std::error_code ec2;
			if (!entry.is_regular_file(ec2))
			{
				continue;
			}
			const auto &p = entry.path();
			if (p.has_extension() && p.extension() == ".vpk")
			{
				return true;
			}
		}
		return false;
	}

	// Remove the entry for `workshopId` from a single ACF section, if present.
	bool PruneIdFromSection(KeyValues *pACF, const char *sectionName, const char *workshopId)
	{
		KeyValues *pSection = pACF->FindKey(sectionName);
		if (!pSection)
		{
			return false;
		}
		if (!pSection->FindKey(workshopId))
		{
			return false;
		}
		return pSection->FindAndDeleteSubKey(workshopId);
	}

	bool PruneACFEntryForId(const std::string &workshopId, CSteamGameServerAPIContext &steamAPI)
	{
		if (!g_pFullFileSystem)
		{
			return false;
		}

		const std::string workshopRoot = WorkshopRootAbs();
		const std::string acfPath = workshopRoot + "/appworkshop_730.acf";

		KeyValues *pACF = new KeyValues("AppWorkshop");
		KeyValues::AutoDelete autoDelete(pACF);
		if (!pACF->LoadFromFile(g_pFullFileSystem, acfPath.c_str(), "GAME"))
		{
			// No ACF means Steam doesn't believe the addon is installed; nothing to do.
			return false;
		}

		bool removedInstalled = PruneIdFromSection(pACF, "WorkshopItemsInstalled", workshopId.c_str());
		bool removedDetails = PruneIdFromSection(pACF, "WorkshopItemDetails", workshopId.c_str());
		if (!removedInstalled && !removedDetails)
		{
			return false;
		}

		pACF->SaveToFile(g_pFullFileSystem, acfPath.c_str(), "GAME");
		if (steamAPI.SteamUGC())
		{
			steamAPI.SteamUGC()->BInitWorkshopForGameServer(730, const_cast<char *>(workshopRoot.c_str()));
		}
		return true;
	}

} // namespace

namespace mmu
{
	namespace workshop
	{

		bool ResolveManager()
		{
			if (g_pWorkshopMgr)
			{
				return true;
			}
			if (!gamesystem::Ready())
			{
				return false;
			}
			g_pWorkshopMgr = static_cast<CDedicatedServerWorkshopManager *>(gamesystem::FindByName("DedicatedServerWorkshopManager"));
			if (g_pWorkshopMgr)
			{
				MMU_LOG_INFO("Workshop: engine workshop manager resolved.\n");
			}
			return g_pWorkshopMgr != nullptr;
		}

		bool ManagerReady()
		{
			return g_pWorkshopMgr != nullptr;
		}

		bool IsMapInstalled(uint64_t fileId)
		{
			if (!ResolveManager())
			{
				return false;
			}

			// Manual index walk so the engine's comparator is never invoked.
			const auto &maps = g_pWorkshopMgr->m_mapLoadedWorkshopMaps;
			for (int i = 0; i < maps.MaxElement(); i++)
			{
				if (maps.IsValidIndex(i) && maps.Key(i) == fileId)
				{
					return true;
				}
			}
			return false;
		}

		std::vector<uint64_t> InstalledMapIds()
		{
			std::vector<uint64_t> out;
			if (!ResolveManager())
			{
				return out;
			}

			const auto &maps = g_pWorkshopMgr->m_mapLoadedWorkshopMaps;
			for (int i = 0; i < maps.MaxElement(); i++)
			{
				if (maps.IsValidIndex(i))
				{
					out.push_back(maps.Key(i));
				}
			}
			return out;
		}

		bool IsReady(uint64_t fileId, CSteamGameServerAPIContext &steamAPI)
		{
			if (fileId == 0)
			{
				return false;
			}
			if (IsMapInstalled(fileId))
			{
				return true;
			}
			if (steamAPI.SteamUGC() && (steamAPI.SteamUGC()->GetItemState(fileId) & k_EItemStateInstalled) != 0)
			{
				return true;
			}
			return WorkshopFolderHasVPK(std::to_string(fileId));
		}

		bool StartDownload(uint64_t fileId, CSteamGameServerAPIContext &steamAPI)
		{
			if (fileId == 0 || !steamAPI.SteamUGC())
			{
				return false;
			}
			return steamAPI.SteamUGC()->DownloadItem(fileId, true);
		}

		bool DownloadProgress(uint64_t fileId, CSteamGameServerAPIContext &steamAPI, uint64_t &done, uint64_t &total)
		{
			done = 0;
			total = 0;
			if (fileId == 0 || !steamAPI.SteamUGC())
			{
				return false;
			}
			return steamAPI.SteamUGC()->GetItemDownloadInfo(fileId, &done, &total) && total > 0;
		}

	} // namespace workshop

	bool EnsureWorkshopMapReady(const std::string &workshopId, CSteamGameServerAPIContext &steamAPI)
	{
		if (workshopId.empty())
		{
			return false;
		}

		// Engine registry is the authority: it lists the map only once its addon path resolved, so a listed map needs no ACF surgery.
		uint64_t fileId = std::strtoull(workshopId.c_str(), nullptr, 10);
		if (fileId != 0 && workshop::IsMapInstalled(fileId))
		{
			return false;
		}

		if (WorkshopFolderHasVPK(workshopId))
		{
			return false; // already good
		}

		MMU_LOG_INFO("Workshop addon %s has no .vpk on disk; pruning stale ACF entry so Steam will re-download.\n", workshopId.c_str());
		return PruneACFEntryForId(workshopId, steamAPI);
	}

} // namespace mmu
