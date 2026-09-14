#ifndef _INCLUDE_MMU_VOICE_BLOCK_H_
#define _INCLUDE_MMU_VOICE_BLOCK_H_

namespace mmu
{
	// Drops a client's voice packets on arrival (CServerSideClient::ProcessVoiceData).
	// Who hears whom is never touched, so a block applies on the next packet and lifting it needs no restoring.
	namespace voiceblock
	{
		using ShouldBlock = bool (*)(int slot);

		// Call from plugin Load. `engineModuleAnchor` is any pointer inside engine2, e.g. g_pEngine.
		// Returns false, and blocks nothing, when the CServerSideClient vtable can't be found.
		bool Init(const void *engineModuleAnchor, ShouldBlock shouldBlock);

		// Call from plugin Unload.
		void Shutdown();
	} // namespace voiceblock
} // namespace mmu

#endif // _INCLUDE_MMU_VOICE_BLOCK_H_
