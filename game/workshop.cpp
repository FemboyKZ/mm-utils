#include "game/workshop.h"
#include "sdk/plugin_globals.h"
#include "sdk/sigscan.h"
#include "utils/log.h"

#include <filesystem.h>
#include <interfaces/interfaces.h>
#include <KeyValues.h>
#include <tier1/bufferstring.h>
#include <tier1/convar.h>
#include <tier1/utlmap.h>
#include <tier1/utlstring.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace
{
	template<typename T>
	using WorkshopTree = CUtlOrderedMap<uint64_t, T, bool (*)(const uint64_t &, const uint64_t &), int>;

	struct WorkshopMapInfo_t
	{
		CBufferStringN<200> m_szFilePath;
		CUtlString m_szMapName;
	};

	static_assert(sizeof(WorkshopMapInfo_t) == 216, "WorkshopMapInfo_t layout drifted from the engine's");

	class CDedicatedServerWorkshopManager
	{
	public:
		void *m_pVTable;                                           // 0x00
		const char *m_pszGameSystemName;                           // 0x08
		uint8_t pad0[0x60];                                        // 0x10, UGC path resolver and SteamWorks callbacks
		WorkshopTree<void *> m_requestedMaps;                      // 0x70
		WorkshopTree<WorkshopMapInfo_t *> m_mapLoadedWorkshopMaps; // 0x98
		WorkshopTree<bool> m_mapStatus;                            // 0xC0
		uint64_t m_nRequestedSharedFileId;                         // 0xE8
		uint64_t m_nRequestedCollectionSharedFileId;               // 0xF0
		bool m_bInitialized;                                       // 0xF8
	};

	static_assert(sizeof(WorkshopTree<void *>) == 0x28, "CUtlOrderedMap layout drifted from the engine's");
	static_assert(offsetof(CDedicatedServerWorkshopManager, m_mapLoadedWorkshopMaps) == 0x98, "workshop manager layout drifted");
	static_assert(sizeof(CDedicatedServerWorkshopManager) == 0x100, "workshop manager layout drifted");

	CDedicatedServerWorkshopManager *g_pWorkshopMgr = nullptr;

	std::string WorkshopRootAbs()
	{
		static std::string cached;
		if (!cached.empty())
		{
			return cached;
		}

		char abs[1024] = {};
		V_MakeAbsolutePath(abs, sizeof(abs), "./steamapps/workshop");
		cached = abs[0] ? abs : "steamapps/workshop";
		MMU_LOG_INFO("Workshop: content root resolved to '%s'.\n", cached.c_str());
		return cached;
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

	const uint32 kTransferStates = k_EItemStateDownloading | k_EItemStateDownloadPending;

	// How long the old map may run on after the engine drops a host_workshop_map before the change counts as failed.
	// Also covers the frame the command sits queued.
	const double kChangeGraceSecs = 10.0;

	// True while Steam is fetching any addon the ACF or the content folder knows about.
	bool AnyDownloadInFlight(KeyValues *pACF, ISteamUGC *pUGC)
	{
		auto inFlight = [pUGC](const char *name)
		{
			uint64_t id = std::strtoull(name, nullptr, 10);
			return id != 0 && (pUGC->GetItemState(id) & kTransferStates) != 0;
		};

		if (KeyValues *pInstalled = pACF->FindKey("WorkshopItemsInstalled"))
		{
			for (KeyValues *pItem = pInstalled->GetFirstSubKey(); pItem; pItem = pItem->GetNextKey())
			{
				if (inFlight(pItem->GetName()))
				{
					return true;
				}
			}
		}

		std::error_code ec;
		for (const auto &entry : fs::directory_iterator(fs::path(WorkshopRootAbs()) / "content" / "730", ec))
		{
			if (inFlight(entry.path().filename().string().c_str()))
			{
				return true;
			}
		}
		return false;
	}

	// wscleaner deletes unused addon folders once per level, and a map downloaded ahead of its host_workshop_map is one.
	// True if the id was added. One the server owner listed is left alone, taking it back out would drop theirs.
	bool ExcludeFromCleaner(uint64_t fileId)
	{
		CConVarRef<CUtlString> exclude("wscleaner_exclude");
		if (!exclude.IsValidRef() || !exclude.IsConVarDataAvailable())
		{
			return false;
		}

		const std::string id = std::to_string(fileId);
		const CUtlString list = exclude.Get();
		const std::string listed = std::string(",") + (list.Get() ? list.Get() : "") + ",";
		if (listed.find("," + id + ",") != std::string::npos)
		{
			return false;
		}
		g_pEngine->ServerCommand(("wscleaner_exclude_add " + id + "\n").c_str());
		return true;
	}

	// Manual index walk so the engine's comparator is never invoked.
	template<typename T>
	int FindKey(const WorkshopTree<T> &tree, uint64_t fileId)
	{
		for (int i = 0; i < tree.MaxElement(); i++)
		{
			if (tree.IsValidIndex(i) && tree.Key(i) == fileId)
			{
				return i;
			}
		}
		return tree.InvalidIndex();
	}

	// A stale record keeps ds_workshop_changelevel, changelevel by map name and the id to vpk lookup pointing at a deleted file.
	void ForgetLoadedMap(uint64_t fileId)
	{
		auto &maps = g_pWorkshopMgr->m_mapLoadedWorkshopMaps;
		int index = FindKey(maps, fileId);
		if (index == maps.InvalidIndex())
		{
			return;
		}

		// The engine allocated the record from tier0, which is where memoverride sends this delete.
		WorkshopMapInfo_t *pInfo = maps.Element(index);
		maps.RemoveAt(index);
		delete pInfo;
		MMU_LOG_INFO("Workshop: dropped the engine's record of %llu.\n", static_cast<unsigned long long>(fileId));
	}

	// Clears every record of an addon that has no vpk on disk: the engine's, the leftover folder and Steam's.
	// Returns true if Steam's ACF entry was pruned.
	bool DropStaleAddon(const std::string &workshopId)
	{
		if (!g_pFullFileSystem)
		{
			return false;
		}

		// The folder is deleted by this id, so it has to be the same one the caller checked for a vpk.
		uint64_t fileId = std::strtoull(workshopId.c_str(), nullptr, 10);
		if (fileId == 0 || std::to_string(fileId) != workshopId)
		{
			return false;
		}

		// Files the engine is still fetching are not stale.
		if (mmu::workshop::IsRequestPending(fileId))
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

		// Steam refuses the reload below while its download job runs,
		// and that job writes its own item list back over the ACF when it finishes.
		ISteamUGC *pUGC = SteamGameServerUGC();
		if (pUGC && AnyDownloadInFlight(pACF, pUGC))
		{
			MMU_LOG_WARN("Workshop: a download is running, leaving the ACF entry for %s in place.\n", workshopId.c_str());
			return false;
		}

		if (mmu::workshop::ResolveManager())
		{
			ForgetLoadedMap(fileId);
		}

		std::error_code ec;
		if (fs::remove_all(fs::path(workshopRoot) / "content" / "730" / workshopId, ec) && !ec)
		{
			MMU_LOG_INFO("Workshop: removed the vpk-less folder of %s.\n", workshopId.c_str());
		}

		bool removedInstalled = PruneIdFromSection(pACF, "WorkshopItemsInstalled", workshopId.c_str());
		bool removedDetails = PruneIdFromSection(pACF, "WorkshopItemDetails", workshopId.c_str());
		if (!removedInstalled && !removedDetails)
		{
			return false;
		}

		pACF->SaveToFile(g_pFullFileSystem, acfPath.c_str(), "GAME");
		MMU_LOG_INFO("Workshop: pruned the stale ACF entry of %s so Steam fetches it again.\n", workshopId.c_str());
		if (pUGC && !pUGC->BInitWorkshopForGameServer(730, const_cast<char *>(workshopRoot.c_str())))
		{
			MMU_LOG_WARN("Workshop: Steam refused to reload its workshop state, %s may still be reported as installed.\n", workshopId.c_str());
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
			// Searched once, a miss would otherwise cost a module scan per call.
			static bool searched = false;
			if (searched || !g_pServerGameDLL)
			{
				return g_pWorkshopMgr != nullptr;
			}
			searched = true;

			void *vtable = sig::FindVirtualTable(g_pServerGameDLL, "CDedicatedServerWorkshopManager");
			auto *pManager = static_cast<CDedicatedServerWorkshopManager *>(vtable ? sig::FindObjectByVTable(g_pServerGameDLL, vtable) : nullptr);
			if (!pManager || !pManager->m_pszGameSystemName || strcmp(pManager->m_pszGameSystemName, "DedicatedServerWorkshopManager") != 0)
			{
				MMU_LOG_WARN("Workshop: engine workshop manager not found, stale map records stay and changes are not watched.\n");
				return false;
			}

			g_pWorkshopMgr = pManager;
			MMU_LOG_INFO("Workshop: engine workshop manager resolved.\n");
			return true;
		}

		bool IsRequestPending(uint64_t fileId)
		{
			if (!ResolveManager())
			{
				return false;
			}
			const auto &requests = g_pWorkshopMgr->m_requestedMaps;
			return FindKey(requests, fileId) != requests.InvalidIndex();
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

		// Files on disk, and nothing else.
		// Steam's k_EItemStateInstalled reflects the ACF, which outlives files a cleaner deleted,
		// and the engine's registry is read through a struct layout we reconstructed by hand.
		// A false positive here hands the engine an empty addon and drops it on the "error" map, so only real files count.
		// Erring the other way just costs a DownloadItem that returns immediately.
		bool IsReady(uint64_t fileId)
		{
			return fileId != 0 && WorkshopFolderHasVPK(std::to_string(fileId));
		}

		// Only meaningful once StartDownload has been called for this id:
		// the ACF was pruned and Steam re-asked, so its answer is fresh rather than inherited.
		// Lets a wait finish even where the folder scan cannot see the content root.
		bool DownloadSettled(uint64_t fileId)
		{
			// Installed stays set and a partial vpk can sit on disk while a transfer is still running.
			uint32 state = SteamGameServerUGC() ? SteamGameServerUGC()->GetItemState(fileId) : 0;
			if (state & kTransferStates)
			{
				return false;
			}
			return IsReady(fileId) || (state & k_EItemStateInstalled) != 0;
		}

		bool StartDownload(uint64_t fileId)
		{
			if (fileId == 0 || !SteamGameServerUGC())
			{
				return false;
			}

			// Installed with no files and no transfer means the ACF prune did not take, so DownloadItem would fetch nothing.
			uint32 state = SteamGameServerUGC()->GetItemState(fileId);
			if ((state & k_EItemStateInstalled) && !(state & kTransferStates) && !IsReady(fileId))
			{
				MMU_LOG_WARN("Workshop: Steam still reports %llu as installed without files, not starting a download.\n",
							 static_cast<unsigned long long>(fileId));
				return false;
			}
			return SteamGameServerUGC()->DownloadItem(fileId, true);
		}

		bool DownloadProgress(uint64_t fileId, uint64_t &done, uint64_t &total)
		{
			done = 0;
			total = 0;
			if (fileId == 0 || !SteamGameServerUGC())
			{
				return false;
			}

			uint64 steamDone = 0;
			uint64 steamTotal = 0;
			if (!SteamGameServerUGC()->GetItemDownloadInfo(fileId, &steamDone, &steamTotal) || steamTotal == 0)
			{
				return false;
			}

			done = static_cast<uint64_t>(steamDone);
			total = static_cast<uint64_t>(steamTotal);
			return true;
		}

		bool PendingDownload::Begin(uint64_t fileId, float timeoutSecs, float announceInterval)
		{
			ISteamUGC *pUGC = SteamGameServerUGC();
			if (timeoutSecs <= 0.0f || fileId == 0 || !pUGC)
			{
				return false;
			}
			Clear();

			PublishedFileId_t id = fileId;
			UGCQueryHandle_t hQuery = pUGC->CreateQueryUGCDetailsRequest(&id, 1);
			SteamAPICall_t hCall = hQuery != k_UGCQueryHandleInvalid ? pUGC->SendQueryUGCRequest(hQuery) : k_uAPICallInvalid;
			if (hCall == k_uAPICallInvalid)
			{
				if (hQuery != k_UGCQueryHandleInvalid)
				{
					pUGC->ReleaseQueryUGCRequest(hQuery);
				}
				return false;
			}

			const double now = Plat_FloatTime();
			m_phase = Phase::Querying;
			m_fileId = fileId;
			m_pQueryUGC = pUGC;
			m_hQuery = hQuery;
			m_hCall = hCall;
			m_deadline = now + timeoutSecs;
			m_announceInterval = announceInterval;
			m_nextAnnounce = now + announceInterval;
			return true;
		}

		void PendingDownload::WatchEngine(uint64_t fileId, float announceInterval)
		{
			// Not Clear, the map just downloaded stays off wscleaner's list until the level has changed.
			ResetState();
			if (fileId == 0 || !ResolveManager())
			{
				return;
			}

			const double now = Plat_FloatTime();
			m_phase = Phase::Hosting;
			m_fileId = fileId;
			m_deadline = now + kChangeGraceSecs;
			m_announceInterval = announceInterval;
			m_nextAnnounce = now + announceInterval;
		}

		PendingDownload::Status PendingDownload::Poll()
		{
			const double now = Plat_FloatTime();

			switch (m_phase)
			{
				case Phase::Idle:
					return Status::Idle;

				case Phase::Querying:
				{
					ISteamUtils *pUtils = SteamGameServerUtils();
					bool failed = false;
					if (!pUtils || !pUtils->IsAPICallCompleted(m_hCall, &failed))
					{
						break;
					}

					SteamUGCQueryCompleted_t result = {};
					SteamUGCDetails_t details = {};
					bool found = !failed && pUtils->GetAPICallResult(m_hCall, &result, sizeof(result), SteamUGCQueryCompleted_t::k_iCallback, &failed)
								 && !failed && result.m_eResult == k_EResultOK && result.m_unNumResultsReturned > 0
								 && m_pQueryUGC->GetQueryUGCResult(m_hQuery, 0, &details) && details.m_eResult == k_EResultOK;
					ReleaseQuery();

					if (!found || details.m_nConsumerAppID != 730 || details.m_eFileType == k_EWorkshopFileTypeCollection || details.m_bBanned)
					{
						Clear();
						return Status::Rejected;
					}

					// Control characters would let a title inject chat colours.
					for (const char *c = details.m_rgchTitle; *c; ++c)
					{
						if (static_cast<unsigned char>(*c) >= ' ')
						{
							m_title += *c;
						}
					}

					// Without this Steam may still think a deleted map is installed and download nothing.
					EnsureWorkshopMapReady(std::to_string(m_fileId));
					if (!StartDownload(m_fileId))
					{
						Clear();
						return Status::StartFailed;
					}
					if (ExcludeFromCleaner(m_fileId))
					{
						m_cleanerExcluded = m_fileId;
					}
					m_downloadResult.Register(this, &PendingDownload::OnDownloadResult);
					m_phase = Phase::Downloading;
					return Status::Started;
				}

				case Phase::Downloading:
					if (m_downloadFailed)
					{
						Clear();
						return Status::DownloadFailed;
					}
					if (DownloadSettled(m_fileId))
					{
						// Not Clear, the exclusion has to outlast the caller's host_workshop_map.
						ResetState();
						return Status::Settled;
					}
					break;

				case Phase::Hosting:
					// Still the engine's turn, so the verdict keeps moving out.
					if (IsRequestPending(m_fileId))
					{
						m_deadline = now + kChangeGraceSecs;
					}
					break;
			}

			if (now >= m_deadline)
			{
				const bool hosting = m_phase == Phase::Hosting;
				Clear();
				return hosting ? Status::ChangeFailed : Status::TimedOut;
			}
			if (now >= m_nextAnnounce)
			{
				m_nextAnnounce = now + m_announceInterval;
				return Status::Announce;
			}
			return Status::Waiting;
		}

		void PendingDownload::ReleaseQuery()
		{
			if (m_hQuery != k_UGCQueryHandleInvalid)
			{
				m_pQueryUGC->ReleaseQueryUGCRequest(m_hQuery);
			}
			m_pQueryUGC = nullptr;
			m_hQuery = k_UGCQueryHandleInvalid;
			m_hCall = k_uAPICallInvalid;
		}

		void PendingDownload::OnDownloadResult(DownloadItemResult_t *pResult)
		{
			if (m_phase != Phase::Downloading || pResult->m_nPublishedFileId != m_fileId || pResult->m_eResult == k_EResultOK)
			{
				return;
			}
			MMU_LOG_WARN("Workshop: Steam failed to download %llu, result %d.\n", static_cast<unsigned long long>(m_fileId),
						 static_cast<int>(pResult->m_eResult));
			m_downloadFailed = true;
		}

		void PendingDownload::ResetState()
		{
			ReleaseQuery();
			// Unregister calls into Steam even when nothing is registered.
			if (m_phase == Phase::Downloading)
			{
				m_downloadResult.Unregister();
			}
			m_downloadFailed = false;
			m_phase = Phase::Idle;
			m_fileId = 0;
			m_title.clear();
		}

		void PendingDownload::Clear()
		{
			ResetState();
			if (m_cleanerExcluded != 0)
			{
				g_pEngine->ServerCommand(("wscleaner_exclude_remove " + std::to_string(m_cleanerExcluded) + "\n").c_str());
				m_cleanerExcluded = 0;
			}
		}

	} // namespace workshop

	bool EnsureWorkshopMapReady(const std::string &workshopId)
	{
		if (workshopId.empty())
		{
			return false;
		}

		// Disk is the only authority here, for the reasons on workshop::IsReady.
		if (WorkshopFolderHasVPK(workshopId))
		{
			return false; // already good
		}

		return DropStaleAddon(workshopId);
	}

} // namespace mmu
