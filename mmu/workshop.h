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
		// Requires mmu::gamesystem::Resolve to have succeeded.
		// Resolution is lazy and cached, every query below attempts it first.
		bool ResolveManager();
		bool ManagerReady();

		// True when the engine's own registry lists the workshop map as loaded/mounted.
		bool IsMapInstalled(uint64_t fileId);

		// All workshop file ids the engine currently lists as loaded.
		std::vector<uint64_t> InstalledMapIds();

		// True only when the addon's .vpk is actually on disk.
		// Neither Steam's state nor the engine registry is consulted, since both outlive deleted files.
		bool IsReady(uint64_t fileId, CSteamGameServerAPIContext &steamAPI);

		// True once a download started by StartDownload has landed.
		// Trusts Steam's state as well as disk, which IsReady deliberately does not.
		bool DownloadSettled(uint64_t fileId, CSteamGameServerAPIContext &steamAPI);

		// Asks Steam to fetch the addon. Completion shows up through DownloadSettled.
		bool StartDownload(uint64_t fileId, CSteamGameServerAPIContext &steamAPI);

		// Bytes of an in-flight download. False when Steam reports no transfer.
		bool DownloadProgress(uint64_t fileId, CSteamGameServerAPIContext &steamAPI, uint64_t &done, uint64_t &total);

		// Wait-for-download state for a workshop map change. Messaging stays with the caller.
		class PendingDownload
		{
		public:
			enum class Status
			{
				Idle,
				Waiting,
				Announce, // still waiting, progress interval elapsed
				Settled,  // downloaded, change map now
				TimedOut,
			};

			// Returns false and arms nothing when timeoutSecs <= 0.
			bool Begin(uint64_t fileId, float timeoutSecs, float now, float announceInterval = 10.0f)
			{
				if (timeoutSecs <= 0.0f)
				{
					return false;
				}
				m_active = true;
				m_fileId = fileId;
				m_deadline = now + timeoutSecs;
				m_announceInterval = announceInterval;
				m_nextAnnounce = now + announceInterval;
				return true;
			}

			// Settled and TimedOut clear the state, so each is returned once.
			Status Poll(float now, CSteamGameServerAPIContext &steamAPI)
			{
				if (!m_active)
				{
					return Status::Idle;
				}
				if (DownloadSettled(m_fileId, steamAPI))
				{
					Clear();
					return Status::Settled;
				}
				if (now >= m_deadline)
				{
					Clear();
					return Status::TimedOut;
				}
				if (now >= m_nextAnnounce)
				{
					m_nextAnnounce = now + m_announceInterval;
					return Status::Announce;
				}
				return Status::Waiting;
			}

			// False when Steam reports no transfer.
			bool Percent(CSteamGameServerAPIContext &steamAPI, int &outPercent) const
			{
				uint64_t done = 0, total = 0;
				if (!m_active || !DownloadProgress(m_fileId, steamAPI, done, total) || total == 0)
				{
					return false;
				}
				outPercent = static_cast<int>((done * 100) / total);
				return true;
			}

			bool Active() const
			{
				return m_active;
			}

			void Clear()
			{
				m_active = false;
				m_fileId = 0;
			}

		private:
			bool m_active = false;
			uint64_t m_fileId = 0;
			float m_deadline = 0.0f;
			float m_nextAnnounce = 0.0f;
			float m_announceInterval = 10.0f;
		};

	} // namespace workshop

	// Ensure a workshop map can be downloaded cleanly at map change.
	// Otherwise, if the addon has no .vpk on disk, prune its stale ACF entry
	// (WorkshopItemsInstalled + WorkshopItemDetails in appworkshop_730.acf)
	// so Steam re-downloads it, then ask SteamUGC to re-read the file.
	// `steamAPI` is the plugin's game-server API context, used for the re-read.
	// Returns true if a stale entry was pruned.
	bool EnsureWorkshopMapReady(const std::string &workshopId, CSteamGameServerAPIContext &steamAPI);
} // namespace mmu

#endif // _INCLUDE_MMU_WORKSHOP_H_
