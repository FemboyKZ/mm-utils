#ifndef _INCLUDE_MMU_RECIPIENT_FILTER_H_
#define _INCLUDE_MMU_RECIPIENT_FILTER_H_

#include <const.h>
#include <irecipientfilter.h>

// Both filters below drop out of range slots rather than trust the caller.
static inline bool MMU_IsValidRecipientSlot(int slot)
{
	return slot >= 0 && slot < ABSOLUTE_PLAYER_LIMIT;
}

// Reliable recipient filter targeting exactly one player slot.
class CSingleRecipientFilter : public IRecipientFilter
{
public:
	explicit CSingleRecipientFilter(int slot)
	{
		if (MMU_IsValidRecipientSlot(slot))
		{
			m_recipients.Set(slot);
		}
	}

	~CSingleRecipientFilter() override {}

	NetChannelBufType_t GetNetworkBufType() const override
	{
		return BUF_RELIABLE;
	}

	bool IsInitMessage() const override
	{
		return false;
	}

	const CPlayerBitVec &GetRecipients() const override
	{
		return m_recipients;
	}

	CPlayerSlot GetPredictedPlayerSlot() const override
	{
		return CPlayerSlot(-1);
	}

private:
	CPlayerBitVec m_recipients;
};

// Reliable recipient filter with manually added slots.
class CMultiRecipientFilter : public IRecipientFilter
{
public:
	CMultiRecipientFilter() {}

	~CMultiRecipientFilter() override {}

	void AddRecipient(int slot)
	{
		if (MMU_IsValidRecipientSlot(slot))
		{
			m_recipients.Set(slot);
		}
	}

	NetChannelBufType_t GetNetworkBufType() const override
	{
		return BUF_RELIABLE;
	}

	bool IsInitMessage() const override
	{
		return false;
	}

	const CPlayerBitVec &GetRecipients() const override
	{
		return m_recipients;
	}

	CPlayerSlot GetPredictedPlayerSlot() const override
	{
		return CPlayerSlot(-1);
	}

private:
	CPlayerBitVec m_recipients;
};

#endif // _INCLUDE_MMU_RECIPIENT_FILTER_H_
