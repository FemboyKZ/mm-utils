#include "mmu/log.h"

#include <ISmmPlugin.h>
#include <tier0/icommandline.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>

extern ISmmAPI *g_SMAPI;

namespace mmu
{
	const char *g_logTag = "mmu";
}

namespace
{

	mmu::log::Setup g_setup;
	LoggingChannelID_t g_channel = INVALID_LOGGING_CHANNEL_ID;
	std::string g_channelName;
	std::string g_addonName;
	bool g_toFile = false;
	FILE *g_file = nullptr;

	std::filesystem::path LogDir()
	{
		return std::filesystem::path(g_SMAPI->GetBaseDir()) / "addons" / g_addonName / "logs";
	}

	void CloseFile()
	{
		if (g_file)
		{
			fclose(g_file);
			g_file = nullptr;
		}
	}

	void OpenFile()
	{
		if (g_file || g_addonName.empty())
		{
			return;
		}

		std::filesystem::path dir = LogDir();
		std::error_code ec;
		if (!std::filesystem::exists(dir, ec))
		{
			std::filesystem::create_directories(dir, ec);
			if (ec)
			{
				return;
			}
		}

		char name[256];
		if (g_setup.newFilePerDay)
		{
			std::time_t now = std::time(nullptr);
			std::tm tm {};
#ifdef _WIN32
			localtime_s(&tm, &now);
#else
			localtime_r(&now, &tm);
#endif
			snprintf(name, sizeof(name), "%s_%04d-%02d-%02d.log", g_addonName.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
		}
		else
		{
			snprintf(name, sizeof(name), "%s.log", g_addonName.c_str());
		}

		g_file = fopen((dir / name).string().c_str(), "a");
	}

	void PurgeOldLogs()
	{
		if (g_setup.retentionDays <= 0 || g_addonName.empty())
		{
			return;
		}

		std::error_code ec;
		std::filesystem::path dir = LogDir();
		if (!std::filesystem::exists(dir, ec) || ec)
		{
			return;
		}

		auto cutoff = std::filesystem::file_time_type::clock::now() - std::chrono::hours(24 * g_setup.retentionDays);
		for (const auto &entry : std::filesystem::directory_iterator(dir, ec))
		{
			if (ec)
			{
				break;
			}
			if (!entry.is_regular_file())
			{
				continue;
			}
			const auto &path = entry.path();
			if (path.extension() != ".log" || path.stem().string().rfind(g_addonName, 0) != 0)
			{
				continue;
			}
			auto writeTime = entry.last_write_time(ec);
			if (ec)
			{
				ec.clear();
				continue;
			}
			if (writeTime < cutoff)
			{
				std::filesystem::remove(path, ec);
				ec.clear();
			}
		}
	}

	// Mirrors this plugin's channel to the log file.
	class FileListener : public ILoggingListener
	{
	public:
		void Log(const LoggingContext_t *pContext, const tchar *pMessage) override
		{
			if (pContext->m_ChannelID != g_channel || !g_toFile)
			{
				return;
			}

			OpenFile();
			if (!g_file)
			{
				return;
			}

			std::time_t t = std::time(nullptr);
			std::tm tm {};
#ifdef _WIN32
			localtime_s(&tm, &t);
#else
			localtime_r(&t, &tm);
#endif
			char ts[32];
			std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tm);

			size_t msgLen = strlen(pMessage);
			bool needsNewline = (msgLen == 0 || pMessage[msgLen - 1] != '\n');
			fprintf(g_file, "[%s] %s%s", ts, pMessage, needsNewline ? "\n" : "");
			fflush(g_file);
		}
	};

	FileListener g_listener;
	bool g_listenerRegistered = false;

} // namespace

namespace mmu
{
	namespace log
	{

		void Init(const Setup &setup)
		{
			g_setup = setup;
			g_channelName = setup.channelName ? setup.channelName : "mmu";
			g_addonName = setup.addonName ? setup.addonName : "";
			g_toFile = setup.toFile;
			g_logTag = g_channelName.c_str();

			// Channels can't be unregistered, reuse one left over from a previous load.
			g_channel = LoggingSystem_FindChannel(g_channelName.c_str());
			if (g_channel == INVALID_LOGGING_CHANNEL_ID)
			{
				LoggingVerbosity_t verbosity = (g_setup.debug || CommandLine()->FindParm("-debug")) ? LV_MAX : LV_DEFAULT;
				g_channel = LoggingSystem_RegisterLoggingChannel(g_channelName.c_str(), nullptr, 0, verbosity, g_setup.color);
			}

			if (!g_listenerRegistered)
			{
				LoggingSystem_RegisterLoggingListener(&g_listener);
				g_listenerRegistered = true;
			}

			PurgeOldLogs();
		}

		void Shutdown()
		{
			if (g_listenerRegistered)
			{
				LoggingSystem_UnregisterLoggingListener(&g_listener);
				g_listenerRegistered = false;
			}
			CloseFile();
			g_channel = INVALID_LOGGING_CHANNEL_ID;
			g_logTag = "mmu";
		}

		void SetToFile(bool enable)
		{
			g_toFile = enable;
			if (!enable)
			{
				CloseFile();
			}
		}

		void SetRetentionDays(int days)
		{
			g_setup.retentionDays = days;
			PurgeOldLogs();
		}

		bool Ready()
		{
			return g_channel != INVALID_LOGGING_CHANNEL_ID;
		}

		LoggingChannelID_t Channel()
		{
			return g_channel;
		}

		const char *Tag()
		{
			return g_logTag;
		}

	} // namespace log
} // namespace mmu
