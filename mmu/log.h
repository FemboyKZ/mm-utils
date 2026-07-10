#ifndef _INCLUDE_MMU_LOG_H_
#define _INCLUDE_MMU_LOG_H_

#include <tier0/logging.h>

namespace mmu
{
	namespace log
	{
		struct Setup
		{
			// Engine logging channel name and the [Tag] shown in log lines, e.g. "CS2Admin".
			const char *channelName = "mmu";
			// Addon dir name for the log file location, addons/<addonName>/logs. Empty disables file output.
			const char *addonName = "";
			// Channel color in the engine console.
			Color color = Color(37, 162, 255, 255);
			// Mirror log lines to a file.
			bool toFile = false;
			// Date-stamped file name (<addon>_YYYY-MM-DD.log) instead of a single <addon>.log.
			bool newFilePerDay = true;
			// Delete log files older than this many days on Init. 0 keeps everything.
			int retentionDays = 30;
			// Force LV_MAX so MMU_LOG_DEBUG shows without the -debug launch option.
			bool debug = false;
		};

		// Register the logging channel and file listener. Call once from plugin Load.
		void Init(const Setup &setup);

		// Unregister the listener and close the file.
		// Call from plugin Unload, the engine keeps the listener pointer otherwise.
		void Shutdown();

		// Runtime toggle for file mirroring.
		void SetToFile(bool enable);

		// Update the retention window and purge old log files now.
		// Lets plugins apply a config value parsed after Init.
		void SetRetentionDays(int days);

		// True once Init has registered the channel.
		bool Ready();

		LoggingChannelID_t Channel();
		const char *Tag();
	} // namespace log

	// Tag used in bracketed console log lines. Kept in sync with log::Init's channelName.
	// Assign directly only when not using log::Init.
	extern const char *g_logTag;
} // namespace mmu

// Severity macros. Fall back to plain console output before log::Init.
#define MMU_LOG_INFO(fmt, ...) \
	do \
	{ \
		if (mmu::log::Ready()) \
		{ \
			LoggingSystem_Log(mmu::log::Channel(), LS_MESSAGE, "[%s] [INFO] " fmt, mmu::log::Tag(), ##__VA_ARGS__); \
		} \
		else \
		{ \
			Msg("[%s] [INFO] " fmt, mmu::log::Tag(), ##__VA_ARGS__); \
		} \
	} while (0)

#define MMU_LOG_DEBUG(fmt, ...) \
	do \
	{ \
		if (mmu::log::Ready()) \
		{ \
			LoggingSystem_Log(mmu::log::Channel(), LS_DETAILED, "[%s] [DEBUG] " fmt, mmu::log::Tag(), ##__VA_ARGS__); \
		} \
	} while (0)

#define MMU_LOG_WARN(fmt, ...) \
	do \
	{ \
		if (mmu::log::Ready()) \
		{ \
			LoggingSystem_Log(mmu::log::Channel(), LS_WARNING, "[%s] [WARN] " fmt, mmu::log::Tag(), ##__VA_ARGS__); \
		} \
		else \
		{ \
			Warning("[%s] [WARN] " fmt, mmu::log::Tag(), ##__VA_ARGS__); \
		} \
	} while (0)

// Labeled ERROR but sent at LS_WARNING severity.
// LS_ERROR makes the engine's default spew handler exit the server.
#define MMU_LOG_ERROR(fmt, ...) \
	do \
	{ \
		if (mmu::log::Ready()) \
		{ \
			LoggingSystem_Log(mmu::log::Channel(), LS_WARNING, "[%s] [ERROR] " fmt, mmu::log::Tag(), ##__VA_ARGS__); \
		} \
		else \
		{ \
			Warning("[%s] [ERROR] " fmt, mmu::log::Tag(), ##__VA_ARGS__); \
		} \
	} while (0)

#endif // _INCLUDE_MMU_LOG_H_
