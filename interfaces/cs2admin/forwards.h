#ifndef _INCLUDE_ADMIN_FORWARDS_H_
#define _INCLUDE_ADMIN_FORWARDS_H_

#include "interfaces/forward_list.h"

#include <ISmmPlugin.h>

#include <functional>
#include <string>
#include <cstdint>

// Other Metamod plugins can acquire this interface via:
//   ICS2AdminForwards *fwd = (ICS2AdminForwards *)g_SMAPI->MetaFactory(
//       CS2ADMIN_FORWARDS_INTERFACE, nullptr, nullptr);
#define CS2ADMIN_FORWARDS_INTERFACE "ICS2AdminForwards004"

// Opaque handle returned by Register* calls. Pass to the matching Unregister*
// method to remove a callback. 0 is the invalid/sentinel value.
using ForwardHandle = uint32_t;
static constexpr ForwardHandle kInvalidForwardHandle = 0;

// Forward callback types.
// Returning true from blockable callbacks prevents the action.

// Ban/Unban
using OnBanPlayerFn = std::function<bool(int targetSlot, int adminSlot, int timeMinutes, const char *reason)>;
using OnUnbanPlayerFn = std::function<void(const char *authid, int adminSlot)>;

// Mute/Gag/Silence
using OnMutePlayerFn = std::function<bool(int targetSlot, int adminSlot, int timeMinutes, const char *reason)>;
using OnGagPlayerFn = std::function<bool(int targetSlot, int adminSlot, int timeMinutes, const char *reason)>;
using OnSilencePlayerFn = std::function<bool(int targetSlot, int adminSlot, int timeMinutes, const char *reason)>;
using OnUnmutePlayerFn = std::function<void(int targetSlot, int adminSlot)>;
using OnUngagPlayerFn = std::function<void(int targetSlot, int adminSlot)>;
using OnUnsilencePlayerFn = std::function<void(int targetSlot, int adminSlot)>;

// Kick/Slay (blockable)
using OnKickPlayerFn = std::function<bool(int targetSlot, int adminSlot, const char *reason)>;
using OnSlayPlayerFn = std::function<bool(int targetSlot, int adminSlot)>;

// Map change (blockable)
using OnMapChangeFn = std::function<bool(const char *mapName, int adminSlot)>;

// Player lifecycle
using OnClientConnectedFn = std::function<void(int slot, const char *name, uint64_t steamid64, const char *ip)>;
using OnClientDisconnectFn = std::function<void(int slot)>;
using OnClientAuthorizedFn = std::function<void(int slot, const char *authid, uint64_t steamid64)>;

// Report
using OnReportPlayerFn = std::function<void(int reporterSlot, int targetSlot, const char *reason)>;

// Admin check
using OnClientPreAdminCheckFn = std::function<void(int slot)>;

class ICS2AdminForwards
{
public:
	// `owner` is your plugin's g_PLID. Returns a handle for the matching Unregister*, 0 for an empty callback.
	// Call Unregister* from your plugin's Unload(): `meta clear` unloads without telling cs2admin.
	virtual ForwardHandle RegisterOnBanPlayer(PluginId owner, OnBanPlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnUnbanPlayer(PluginId owner, OnUnbanPlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnMutePlayer(PluginId owner, OnMutePlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnGagPlayer(PluginId owner, OnGagPlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnUnmutePlayer(PluginId owner, OnUnmutePlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnUngagPlayer(PluginId owner, OnUngagPlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnReportPlayer(PluginId owner, OnReportPlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnClientPreAdminCheck(PluginId owner, OnClientPreAdminCheckFn callback) = 0;
	virtual ForwardHandle RegisterOnKickPlayer(PluginId owner, OnKickPlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnSlayPlayer(PluginId owner, OnSlayPlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnSilencePlayer(PluginId owner, OnSilencePlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnUnsilencePlayer(PluginId owner, OnUnsilencePlayerFn callback) = 0;
	virtual ForwardHandle RegisterOnMapChange(PluginId owner, OnMapChangeFn callback) = 0;
	virtual ForwardHandle RegisterOnClientConnected(PluginId owner, OnClientConnectedFn callback) = 0;
	virtual ForwardHandle RegisterOnClientDisconnect(PluginId owner, OnClientDisconnectFn callback) = 0;
	virtual ForwardHandle RegisterOnClientAuthorized(PluginId owner, OnClientAuthorizedFn callback) = 0;

	virtual void UnregisterOnBanPlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnUnbanPlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnMutePlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnGagPlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnUnmutePlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnUngagPlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnReportPlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnClientPreAdminCheck(ForwardHandle handle) = 0;

	virtual void UnregisterOnKickPlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnSlayPlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnSilencePlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnUnsilencePlayer(ForwardHandle handle) = 0;
	virtual void UnregisterOnMapChange(ForwardHandle handle) = 0;
	virtual void UnregisterOnClientConnected(ForwardHandle handle) = 0;
	virtual void UnregisterOnClientDisconnect(ForwardHandle handle) = 0;
	virtual void UnregisterOnClientAuthorized(ForwardHandle handle) = 0;
};

class CS2AForwards : public ICS2AdminForwards
{
public:
	// Called from CS2APlugin::Unload() to drop all registered callbacks so that
	// no lambda from another (possibly already-unloaded) plugin can be invoked.
	void Shutdown()
	{
		EachList([](auto &list) { list.Clear(); });
	}

	void DropOwnedBy(PluginId owner)
	{
		EachList([owner](auto &list) { list.Drop(owner); });
	}

	ForwardHandle RegisterOnBanPlayer(PluginId owner, OnBanPlayerFn callback) override
	{
		return m_onBanPlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnUnbanPlayer(PluginId owner, OnUnbanPlayerFn callback) override
	{
		return m_onUnbanPlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnMutePlayer(PluginId owner, OnMutePlayerFn callback) override
	{
		return m_onMutePlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnGagPlayer(PluginId owner, OnGagPlayerFn callback) override
	{
		return m_onGagPlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnUnmutePlayer(PluginId owner, OnUnmutePlayerFn callback) override
	{
		return m_onUnmutePlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnUngagPlayer(PluginId owner, OnUngagPlayerFn callback) override
	{
		return m_onUngagPlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnReportPlayer(PluginId owner, OnReportPlayerFn callback) override
	{
		return m_onReportPlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnClientPreAdminCheck(PluginId owner, OnClientPreAdminCheckFn callback) override
	{
		return m_onClientPreAdminCheck.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnKickPlayer(PluginId owner, OnKickPlayerFn callback) override
	{
		return m_onKickPlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnSlayPlayer(PluginId owner, OnSlayPlayerFn callback) override
	{
		return m_onSlayPlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnSilencePlayer(PluginId owner, OnSilencePlayerFn callback) override
	{
		return m_onSilencePlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnUnsilencePlayer(PluginId owner, OnUnsilencePlayerFn callback) override
	{
		return m_onUnsilencePlayer.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnMapChange(PluginId owner, OnMapChangeFn callback) override
	{
		return m_onMapChange.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnClientConnected(PluginId owner, OnClientConnectedFn callback) override
	{
		return m_onClientConnected.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnClientDisconnect(PluginId owner, OnClientDisconnectFn callback) override
	{
		return m_onClientDisconnect.Add(owner, std::move(callback));
	}

	ForwardHandle RegisterOnClientAuthorized(PluginId owner, OnClientAuthorizedFn callback) override
	{
		return m_onClientAuthorized.Add(owner, std::move(callback));
	}

	void UnregisterOnBanPlayer(ForwardHandle h) override
	{
		m_onBanPlayer.Remove(h);
	}

	void UnregisterOnUnbanPlayer(ForwardHandle h) override
	{
		m_onUnbanPlayer.Remove(h);
	}

	void UnregisterOnMutePlayer(ForwardHandle h) override
	{
		m_onMutePlayer.Remove(h);
	}

	void UnregisterOnGagPlayer(ForwardHandle h) override
	{
		m_onGagPlayer.Remove(h);
	}

	void UnregisterOnUnmutePlayer(ForwardHandle h) override
	{
		m_onUnmutePlayer.Remove(h);
	}

	void UnregisterOnUngagPlayer(ForwardHandle h) override
	{
		m_onUngagPlayer.Remove(h);
	}

	void UnregisterOnReportPlayer(ForwardHandle h) override
	{
		m_onReportPlayer.Remove(h);
	}

	void UnregisterOnClientPreAdminCheck(ForwardHandle h) override
	{
		m_onClientPreAdminCheck.Remove(h);
	}

	void UnregisterOnKickPlayer(ForwardHandle h) override
	{
		m_onKickPlayer.Remove(h);
	}

	void UnregisterOnSlayPlayer(ForwardHandle h) override
	{
		m_onSlayPlayer.Remove(h);
	}

	void UnregisterOnSilencePlayer(ForwardHandle h) override
	{
		m_onSilencePlayer.Remove(h);
	}

	void UnregisterOnUnsilencePlayer(ForwardHandle h) override
	{
		m_onUnsilencePlayer.Remove(h);
	}

	void UnregisterOnMapChange(ForwardHandle h) override
	{
		m_onMapChange.Remove(h);
	}

	void UnregisterOnClientConnected(ForwardHandle h) override
	{
		m_onClientConnected.Remove(h);
	}

	void UnregisterOnClientDisconnect(ForwardHandle h) override
	{
		m_onClientDisconnect.Remove(h);
	}

	void UnregisterOnClientAuthorized(ForwardHandle h) override
	{
		m_onClientAuthorized.Remove(h);
	}

	// Fire forwards. Blockable ones return true if any callback blocked.

	bool FireOnBanPlayer(int targetSlot, int adminSlot, int timeMinutes, const char *reason)
	{
		return m_onBanPlayer.Fire(targetSlot, adminSlot, timeMinutes, reason);
	}

	bool FireOnMutePlayer(int targetSlot, int adminSlot, int timeMinutes, const char *reason)
	{
		return m_onMutePlayer.Fire(targetSlot, adminSlot, timeMinutes, reason);
	}

	bool FireOnGagPlayer(int targetSlot, int adminSlot, int timeMinutes, const char *reason)
	{
		return m_onGagPlayer.Fire(targetSlot, adminSlot, timeMinutes, reason);
	}

	bool FireOnKickPlayer(int targetSlot, int adminSlot, const char *reason)
	{
		return m_onKickPlayer.Fire(targetSlot, adminSlot, reason);
	}

	bool FireOnSlayPlayer(int targetSlot, int adminSlot)
	{
		return m_onSlayPlayer.Fire(targetSlot, adminSlot);
	}

	bool FireOnSilencePlayer(int targetSlot, int adminSlot, int timeMinutes, const char *reason)
	{
		return m_onSilencePlayer.Fire(targetSlot, adminSlot, timeMinutes, reason);
	}

	bool FireOnMapChange(const char *mapName, int adminSlot)
	{
		return m_onMapChange.Fire(mapName, adminSlot);
	}

	void FireOnUnbanPlayer(const char *authid, int adminSlot)
	{
		m_onUnbanPlayer.Fire(authid, adminSlot);
	}

	void FireOnUnmutePlayer(int targetSlot, int adminSlot)
	{
		m_onUnmutePlayer.Fire(targetSlot, adminSlot);
	}

	void FireOnUngagPlayer(int targetSlot, int adminSlot)
	{
		m_onUngagPlayer.Fire(targetSlot, adminSlot);
	}

	void FireOnUnsilencePlayer(int targetSlot, int adminSlot)
	{
		m_onUnsilencePlayer.Fire(targetSlot, adminSlot);
	}

	void FireOnReportPlayer(int reporterSlot, int targetSlot, const char *reason)
	{
		m_onReportPlayer.Fire(reporterSlot, targetSlot, reason);
	}

	void FireOnClientPreAdminCheck(int slot)
	{
		m_onClientPreAdminCheck.Fire(slot);
	}

	void FireOnClientConnected(int slot, const char *name, uint64_t steamid64, const char *ip)
	{
		m_onClientConnected.Fire(slot, name, steamid64, ip);
	}

	void FireOnClientDisconnect(int slot)
	{
		m_onClientDisconnect.Fire(slot);
	}

	void FireOnClientAuthorized(int slot, const char *authid, uint64_t steamid64)
	{
		m_onClientAuthorized.Fire(slot, authid, steamid64);
	}

private:
	template<typename F>
	void EachList(F f)
	{
		f(m_onBanPlayer);
		f(m_onUnbanPlayer);
		f(m_onMutePlayer);
		f(m_onGagPlayer);
		f(m_onUnmutePlayer);
		f(m_onUngagPlayer);
		f(m_onUnsilencePlayer);
		f(m_onReportPlayer);
		f(m_onClientPreAdminCheck);
		f(m_onKickPlayer);
		f(m_onSlayPlayer);
		f(m_onSilencePlayer);
		f(m_onMapChange);
		f(m_onClientConnected);
		f(m_onClientDisconnect);
		f(m_onClientAuthorized);
	}

	mmu::ForwardList<OnBanPlayerFn> m_onBanPlayer;
	mmu::ForwardList<OnUnbanPlayerFn> m_onUnbanPlayer;
	mmu::ForwardList<OnMutePlayerFn> m_onMutePlayer;
	mmu::ForwardList<OnGagPlayerFn> m_onGagPlayer;
	mmu::ForwardList<OnUnmutePlayerFn> m_onUnmutePlayer;
	mmu::ForwardList<OnUngagPlayerFn> m_onUngagPlayer;
	mmu::ForwardList<OnReportPlayerFn> m_onReportPlayer;
	mmu::ForwardList<OnClientPreAdminCheckFn> m_onClientPreAdminCheck;
	mmu::ForwardList<OnKickPlayerFn> m_onKickPlayer;
	mmu::ForwardList<OnSlayPlayerFn> m_onSlayPlayer;
	mmu::ForwardList<OnSilencePlayerFn> m_onSilencePlayer;
	mmu::ForwardList<OnUnsilencePlayerFn> m_onUnsilencePlayer;
	mmu::ForwardList<OnMapChangeFn> m_onMapChange;
	mmu::ForwardList<OnClientConnectedFn> m_onClientConnected;
	mmu::ForwardList<OnClientDisconnectFn> m_onClientDisconnect;
	mmu::ForwardList<OnClientAuthorizedFn> m_onClientAuthorized;
};

extern CS2AForwards g_CS2AForwards;

#endif // _INCLUDE_ADMIN_FORWARDS_H_
