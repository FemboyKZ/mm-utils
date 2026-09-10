#ifndef _INCLUDE_MMU_DISCORD_H_
#define _INCLUDE_MMU_DISCORD_H_

#include "mmu/http_client.h"
#include "mmu/json.h"
#include "mmu/log.h"

#include <string>

namespace mmu
{
	namespace discord
	{
		inline constexpr int kDefaultColor = 0x3498DB;

		// Refuses non-Discord URLs. Fire-and-forget, failures are only logged.
		inline void SendPayload(const std::string &webhookUrl, const std::string &json)
		{
			if (webhookUrl.empty())
			{
				return;
			}

			if (webhookUrl.find("https://discord.com/api/webhooks/") != 0 && webhookUrl.find("https://discordapp.com/api/webhooks/") != 0)
			{
				MMU_LOG_WARN("Discord: Invalid webhook URL (must be a Discord webhook URL).\n");
				return;
			}

			http::Post(webhookUrl, json,
					   [](bool success, std::string)
					   {
						   if (!success)
						   {
							   MMU_LOG_WARN("Discord: Failed to send webhook.\n");
						   }
					   });
		}

		inline void SendText(const std::string &webhookUrl, const char *content)
		{
			if (webhookUrl.empty() || !content || !*content)
			{
				return;
			}
			SendPayload(webhookUrl, "{\"content\":\"" + json::Escape(content) + "\"}");
		}

		inline void SendEmbed(const std::string &webhookUrl, const char *title, const char *description, int color = kDefaultColor,
							  const char *footer = nullptr)
		{
			if (webhookUrl.empty())
			{
				return;
			}

			std::string payload = "{\"embeds\":[{";
			payload += "\"title\":\"" + json::Escape(title ? title : "") + "\",";
			payload += "\"description\":\"" + json::Escape(description ? description : "") + "\",";
			payload += "\"color\":" + std::to_string(color);
			if (footer && *footer)
			{
				payload += ",\"footer\":{\"text\":\"" + json::Escape(footer) + "\"}";
			}
			payload += "}]}";

			SendPayload(webhookUrl, payload);
		}
	} // namespace discord
} // namespace mmu

#endif // _INCLUDE_MMU_DISCORD_H_
