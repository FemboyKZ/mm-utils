#ifndef _INCLUDE_RTV_FORWARDS_H_
#define _INCLUDE_RTV_FORWARDS_H_

#include "interfaces/forward_list.h"

#include <ISmmPlugin.h>

#include <functional>
#include <cstdint>

// Other Metamod plugins can acquire this interface via:
//   ICS2RTVForwards *fwd = (ICS2RTVForwards *)g_SMAPI->MetaFactory(
//       CS2RTV_FORWARDS_INTERFACE, nullptr, nullptr);
#define CS2RTV_FORWARDS_INTERFACE "ICS2RTVForwards002"

// Opaque handle returned by Register* calls.
// Pass to the matching Unregister* method to remove a callback.
// 0 is the invalid/sentinel value.
using RTVForwardHandle = uint32_t;
static constexpr RTVForwardHandle kInvalidRTVForwardHandle = 0;

// Forward callback types.

// Fired when a map vote is about to start.
// isRTV is true for !rtv-triggered votes,
// false for end-of-map / nextmap votes.
// Blockable: return true to prevent the vote from starting.
using OnMapVoteStartFn = std::function<bool(bool isRTV)>;

// Fired when a map vote finishes with a winning map.
// winnerMap is the clean changelevel name.
// Not fired when a vote ends with no result (tie/no-change).
using OnMapVoteEndFn = std::function<void(const char *winnerMap, bool isRTV)>;

// Fired when a winning map has been chosen and the changelevel is scheduled.
// delaySecs is how long until the change executes.
using OnMapChangeScheduledFn = std::function<void(const char *mapName, int delaySecs)>;

class ICS2RTVForwards
{
public:
	// `owner` is your plugin's g_PLID. Returns a handle for the matching Unregister*, 0 for an empty callback.
	// Call Unregister* from your plugin's Unload(): `meta clear` unloads without telling cs2rockthevote.
	virtual RTVForwardHandle RegisterOnMapVoteStart(PluginId owner, OnMapVoteStartFn callback) = 0;
	virtual RTVForwardHandle RegisterOnMapVoteEnd(PluginId owner, OnMapVoteEndFn callback) = 0;
	virtual RTVForwardHandle RegisterOnMapChangeScheduled(PluginId owner, OnMapChangeScheduledFn callback) = 0;

	virtual void UnregisterOnMapVoteStart(RTVForwardHandle handle) = 0;
	virtual void UnregisterOnMapVoteEnd(RTVForwardHandle handle) = 0;
	virtual void UnregisterOnMapChangeScheduled(RTVForwardHandle handle) = 0;
};

class CS2RTVForwards : public ICS2RTVForwards
{
public:
	// Called from CS2RTVPlugin::Unload() to drop all registered callbacks so that
	// no lambda from another (possibly already-unloaded) plugin can be invoked.
	void Shutdown()
	{
		m_onMapVoteStart.Clear();
		m_onMapVoteEnd.Clear();
		m_onMapChangeScheduled.Clear();
	}

	void DropOwnedBy(PluginId owner)
	{
		m_onMapVoteStart.Drop(owner);
		m_onMapVoteEnd.Drop(owner);
		m_onMapChangeScheduled.Drop(owner);
	}

	RTVForwardHandle RegisterOnMapVoteStart(PluginId owner, OnMapVoteStartFn callback) override
	{
		return m_onMapVoteStart.Add(owner, std::move(callback));
	}

	RTVForwardHandle RegisterOnMapVoteEnd(PluginId owner, OnMapVoteEndFn callback) override
	{
		return m_onMapVoteEnd.Add(owner, std::move(callback));
	}

	RTVForwardHandle RegisterOnMapChangeScheduled(PluginId owner, OnMapChangeScheduledFn callback) override
	{
		return m_onMapChangeScheduled.Add(owner, std::move(callback));
	}

	void UnregisterOnMapVoteStart(RTVForwardHandle h) override
	{
		m_onMapVoteStart.Remove(h);
	}

	void UnregisterOnMapVoteEnd(RTVForwardHandle h) override
	{
		m_onMapVoteEnd.Remove(h);
	}

	void UnregisterOnMapChangeScheduled(RTVForwardHandle h) override
	{
		m_onMapChangeScheduled.Remove(h);
	}

	// Fire forwards.

	// Returns true if any callback blocked the vote.
	bool FireOnMapVoteStart(bool isRTV)
	{
		return m_onMapVoteStart.Fire(isRTV);
	}

	void FireOnMapVoteEnd(const char *winnerMap, bool isRTV)
	{
		m_onMapVoteEnd.Fire(winnerMap, isRTV);
	}

	void FireOnMapChangeScheduled(const char *mapName, int delaySecs)
	{
		m_onMapChangeScheduled.Fire(mapName, delaySecs);
	}

private:
	mmu::ForwardList<OnMapVoteStartFn> m_onMapVoteStart;
	mmu::ForwardList<OnMapVoteEndFn> m_onMapVoteEnd;
	mmu::ForwardList<OnMapChangeScheduledFn> m_onMapChangeScheduled;
};

extern CS2RTVForwards g_CS2RTVForwards;

#endif // _INCLUDE_RTV_FORWARDS_H_
