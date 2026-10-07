#ifndef _INCLUDE_MMU_WORKSHOP_H_
#define _INCLUDE_MMU_WORKSHOP_H_

#include <steam/steam_gameserver.h>

#include <cstdint>
#include <string>
#include <vector>

namespace mmu
{
	namespace workshop
	{
		// Engine-native workshop registry, the CDedicatedServerWorkshopManager game system.
		// Found through its RTTI in g_pServerGameDLL's module, lazily and once. Every query below attempts it first.
		bool ResolveManager();

		// All workshop file ids the engine currently lists as loaded.
		std::vector<uint64_t> InstalledMapIds();

		// True only when the addon's .vpk is actually on disk.
		// Neither Steam's state nor the engine registry is consulted, since both outlive deleted files.
		bool IsReady(uint64_t fileId);

		// True once a download started by StartDownload has landed.
		// Trusts Steam's state as well as disk, which IsReady deliberately does not.
		// Never true while Steam still reports a transfer for the addon.
		bool DownloadSettled(uint64_t fileId, CSteamGameServerAPIContext &steamAPI);

		// Asks Steam to fetch the addon. Completion shows up through DownloadSettled.
		// False when Steam still calls a fileless addon installed, since that download would fetch nothing.
		bool StartDownload(uint64_t fileId, CSteamGameServerAPIContext &steamAPI);

		// Bytes of an in-flight download. False when Steam reports no transfer.
		bool DownloadProgress(uint64_t fileId, CSteamGameServerAPIContext &steamAPI, uint64_t &done, uint64_t &total);

		// True while the engine is still querying, downloading or installing the addon for host_workshop_map.
		// The engine drops the request in the frame it finishes, successfully or not.
		bool IsRequestPending(uint64_t fileId);

		// A workshop map change, from looking the addon up to the engine taking over. Messaging stays with the caller.
		// Times run on the real clock, since curtime stands still on a hibernating server.
		class PendingDownload
		{
		public:
			enum class Status
			{
				Idle,
				Waiting,
				Started,        // Steam confirmed a CS2 map and the download began
				Announce,       // still waiting, progress interval elapsed
				Settled,        // downloaded, change map now
				TimedOut,       // lookup or download still unfinished at the deadline
				Rejected,       // Steam does not know the id, or it is not a CS2 map
				StartFailed,    // no download could be started
				DownloadFailed, // Steam reported the download as failed
				ChangeFailed,   // the map never changed after a watched host_workshop_map
			};

			// Asks Steam what the addon is. The download starts from Poll once the answer says CS2 map.
			// Returns false and arms nothing when timeoutSecs <= 0 or the question could not be sent.
			bool Begin(uint64_t fileId, float timeoutSecs, CSteamGameServerAPIContext &steamAPI, float announceInterval = 10.0f);

			// Call right after issuing host_workshop_map, does nothing when the engine's workshop manager is out of reach.
			// Poll then announces while the engine fetches an update of its own, and returns ChangeFailed if the map never changes.
			void WatchEngine(uint64_t fileId, float announceInterval = 10.0f);

			// Waiting, Started and Announce keep the state. Every other status clears it, so each is returned once.
			Status Poll(CSteamGameServerAPIContext &steamAPI);

			// False when Steam reports no transfer.
			bool Percent(CSteamGameServerAPIContext &steamAPI, int &outPercent) const
			{
				uint64_t done = 0, total = 0;
				if (m_phase == Phase::Idle || !DownloadProgress(m_fileId, steamAPI, done, total) || total == 0)
				{
					return false;
				}
				outPercent = static_cast<int>((done * 100) / total);
				return true;
			}

			// The addon's workshop title, known from Started on.
			const std::string &Title() const
			{
				return m_title;
			}

			// True while the change still waits on the lookup or the download.
			bool Active() const
			{
				return m_phase == Phase::Querying || m_phase == Phase::Downloading;
			}

			// Active, or watching the engine after the change was issued.
			bool Busy() const
			{
				return m_phase != Phase::Idle;
			}

			// Also takes the map off wscleaner's exclude list, so call it on level init even after Poll reported Settled.
			void Clear();

		private:
			enum class Phase
			{
				Idle,
				Querying,
				Downloading,
				Hosting,
			};

			void ReleaseQuery();
			void ResetState();

			// A failed download ends the wait at once instead of running out the timeout.
			STEAM_GAMESERVER_CALLBACK_MANUAL(PendingDownload, OnDownloadResult, DownloadItemResult_t, m_downloadResult);
			bool m_downloadFailed = false;

			Phase m_phase = Phase::Idle;
			uint64_t m_fileId = 0;
			uint64_t m_cleanerExcluded = 0;
			std::string m_title;
			ISteamUGC *m_pQueryUGC = nullptr;
			UGCQueryHandle_t m_hQuery = k_UGCQueryHandleInvalid;
			SteamAPICall_t m_hCall = k_uAPICallInvalid;
			double m_deadline = 0.0;
			double m_nextAnnounce = 0.0;
			float m_announceInterval = 10.0f;
		};

	} // namespace workshop

	// Ensure a workshop map can be downloaded cleanly at map change.
	// Otherwise, if the addon has no .vpk on disk, prune its stale ACF entry
	// (WorkshopItemsInstalled + WorkshopItemDetails in appworkshop_730.acf)
	// so Steam re-downloads it, then ask SteamUGC to re-read the file.
	// The engine's record of the map and any leftover addon folder are dropped with it.
	// `steamAPI` is the plugin's game-server API context, used for the re-read.
	// Returns true if a stale entry was pruned.
	bool EnsureWorkshopMapReady(const std::string &workshopId, CSteamGameServerAPIContext &steamAPI);
} // namespace mmu

#endif // _INCLUDE_MMU_WORKSHOP_H_
