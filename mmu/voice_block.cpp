#include "mmu/voice_block.h"
#include "mmu/log.h"
#include "mmu/server_client.h"

#include <khook.hpp>
#include <netmessages.pb.h>
#include <networksystem/netmessage.h>

namespace
{
	mmu::voiceblock::ShouldBlock s_shouldBlock = nullptr;
	bool s_hooked = false;

	KHook::Return<bool> OnProcessVoiceData(CServerSideClient *client, const CNetMessagePB<CCLCMsg_VoiceData> &)
	{
		if (s_shouldBlock && s_shouldBlock(mmu::serverclient::Slot(client)))
		{
			return {KHook::Action::Supersede, true};
		}
		return {KHook::Action::Ignore, true};
	}

	// Pre, since Supersede in a post callback runs after the packet was already relayed.
	KHook::Virtual<CServerSideClient, bool, const CNetMessagePB<CCLCMsg_VoiceData> &> s_voiceHook(mmu::serverclient::kProcessVoiceDataIndex,
																								  &OnProcessVoiceData, nullptr);
} // namespace

namespace mmu
{
	namespace voiceblock
	{
		bool Init(const void *engineModuleAnchor, ShouldBlock shouldBlock)
		{
			s_shouldBlock = shouldBlock;
			if (s_hooked)
			{
				return true;
			}
			if (!serverclient::Resolve(engineModuleAnchor))
			{
				MMU_LOG_WARN("CServerSideClient vtable not found, voice blocks are not enforced.\n");
				return false;
			}
			s_voiceHook.AddGlobal(serverclient::HookTarget());
			s_hooked = true;
			return true;
		}

		void Shutdown()
		{
			if (s_hooked)
			{
				s_voiceHook.RemoveGlobal(serverclient::HookTarget());
				s_hooked = false;
			}
			s_shouldBlock = nullptr;
		}
	} // namespace voiceblock
} // namespace mmu
