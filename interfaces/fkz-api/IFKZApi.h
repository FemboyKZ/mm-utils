/**
 * FKZ API - Public interface for other Metamod:Source plugins
 *
 * Async wrappers around the FKZ REST API (https://api.femboykz.com).
 *
 * Usage:
 *
 *   #include <IFKZApi.h>
 *
 *   IFKZApi *g_pFKZ = nullptr;
 *
 *   void MyPlugin::AllPluginsLoaded()
 *   {
 *       int ret;
 *       g_pFKZ = (IFKZApi *)g_SMAPI->MetaFactory(FKZ_API_INTERFACE, &ret, nullptr);
 *       if (ret != META_IFACE_OK || !g_pFKZ) return; // fkz-api not loaded
 *
 *       g_pFKZ->GetKzPlayer("76561198000000000", OnPlayer);
 *   }
 *
 *   void OnPlayer(bool success, int statusCode, const char *body, void *data)
 *   {
 *       if (!success)
 *           return;
 *       // `body` is the raw JSON response - parse it with your JSON library.
 *       // It is only valid during this call; copy it to keep it.
 *   }
 *
 * Every method returns true if the request was dispatched (the HTTP result arrives later in the callback)
 * and false if it could not be sent.
 */

#ifndef _INCLUDE_IFKZAPI_H_
#define _INCLUDE_IFKZAPI_H_

/** Versioned interface name passed to ISmmAPI::MetaFactory(). */
#define FKZ_API_INTERFACE "IFKZApi001"

/**
 * Result callback for all FKZ API requests.
 *
 * @param success     True if the HTTP status was 2xx.
 * @param statusCode  HTTP status code (0 on transport error).
 * @param body        Raw response body, null-terminated (never null, empty string when there was no body).
 *                    Valid only for the duration of this call, copy it to retain.
 * @param data        Opaque value passed to the originating request.
 */
typedef void (*FKZ_ResponseCallback)(bool success, int statusCode, const char *body, void *data);

class IFKZApi
{
public:
	/** Interface version (the trailing number in FKZ_API_INTERFACE). */
	virtual int GetInterfaceVersion() = 0;

	// -----------------------------------------------------------------------
	// Core
	// -----------------------------------------------------------------------

	/**
	 * Sends an arbitrary request to any FKZ API endpoint.
	 *
	 * @param method    HTTP verb: GET, POST, PUT, PATCH or DELETE (case-insensitive).
	 * @param path      Endpoint path, e.g. "/global/records?limit=10".
	 *                  Leading slash optional.
	 * @param body      JSON body string for POST/PUT/PATCH, or nullptr.
	 *                  Copied internally.
	 * @param callback  Result callback.
	 * @param data      Opaque value forwarded to the callback.
	 * @return          True if dispatched.
	 */
	virtual bool ApiRequest(const char *method, const char *path, const char *body, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** Copies the configured API base URL (e.g. "https://api.femboykz.com"). Returns its length. */
	virtual int GetApiBase(char *buffer, int maxlength) = 0;

	/** GET /health */
	virtual bool GetHealth(FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** POST /servers/status - submits a live server status report (requires api_key). */
	virtual bool PostServerStatus(const char *body, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** POST /servers/status/hibernate - signals the server is empty/hibernating (requires api_key). */
	virtual bool PostHibernate(const char *body, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// Live servers / players / maps
	// -----------------------------------------------------------------------

	/** GET /servers (limit/offset/sort) */
	virtual bool GetServers(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /servers/{ip} ("ip" or "ip:port") */
	virtual bool GetServer(const char *ip, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** GET /players (limit/offset/sort) */
	virtual bool GetPlayers(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /players/online (limit/offset/sort) */
	virtual bool GetOnlinePlayers(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /players/{steamid} */
	virtual bool GetPlayer(const char *steamid, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** GET /maps (limit/offset/sort) */
	virtual bool GetMaps(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /maps/{mapname} */
	virtual bool GetMap(const char *mapname, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// KZ Global - records
	// -----------------------------------------------------------------------

	/** GET /global/records (limit/offset/sort) */
	virtual bool GetKzRecords(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/records/recent (limit/offset/sort) */
	virtual bool GetKzRecentRecords(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/records/worldrecords (limit/offset/sort) */
	virtual bool GetKzWorldRecords(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/records/leaderboard/{mapname} (limit/offset/sort) */
	virtual bool GetKzLeaderboard(const char *mapname, FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "",
								  void *data = nullptr) = 0;

	/** GET /global/records/{id} */
	virtual bool GetKzRecord(int id, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// KZ Global - players
	// -----------------------------------------------------------------------

	/** GET /global/players (limit/offset/sort) */
	virtual bool GetKzPlayers(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/players/{steamid} */
	virtual bool GetKzPlayer(const char *steamid, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** GET /global/players/{steamid}/records (limit/offset/sort) */
	virtual bool GetKzPlayerRecords(const char *steamid, FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "",
									void *data = nullptr) = 0;

	/** GET /global/players/{steamid}/pbs (limit/offset/sort) */
	virtual bool GetKzPlayerPBs(const char *steamid, FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "",
								void *data = nullptr) = 0;

	/** GET /global/players/{steamid}/completions (limit/offset/sort) */
	virtual bool GetKzPlayerCompletions(const char *steamid, FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "",
										void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// KZ Global - maps
	// -----------------------------------------------------------------------

	/** GET /global/maps (limit/offset/sort) */
	virtual bool GetKzMaps(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/maps/{mapname} */
	virtual bool GetKzMap(const char *mapname, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** GET /global/maps/{mapname}/records (limit/offset/sort) */
	virtual bool GetKzMapRecords(const char *mapname, FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "",
								 void *data = nullptr) = 0;

	/** GET /global/maps/{mapname}/courses (limit/offset/sort) */
	virtual bool GetKzMapCourses(const char *mapname, FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "",
								 void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// KZ Global - servers
	// -----------------------------------------------------------------------

	/** GET /global/servers (limit/offset/sort) */
	virtual bool GetKzServers(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/servers/{id} */
	virtual bool GetKzServer(int id, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// KZ Global - bans
	// -----------------------------------------------------------------------

	/** GET /global/bans (limit/offset/sort) */
	virtual bool GetKzBans(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/bans/active (limit/offset/sort) */
	virtual bool GetKzActiveBans(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /global/bans/{id} */
	virtual bool GetKzBan(int id, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** GET /global/bans/player/{steamid} (limit/offset/sort) */
	virtual bool GetKzPlayerBans(const char *steamid, FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "",
								 void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// KZ Local (CS:GO 128/64 tick)
	// -----------------------------------------------------------------------

	/** GET /local/gokz/maps (limit/offset/sort) */
	virtual bool GetLocalMaps(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /local/gokz/maps/{mapname} */
	virtual bool GetLocalMap(const char *mapname, FKZ_ResponseCallback callback, void *data = nullptr) = 0;

	/** GET /local/gokz/records (limit/offset/sort) */
	virtual bool GetLocalRecords(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /local/gokz/players (limit/offset/sort) */
	virtual bool GetLocalPlayers(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	// -----------------------------------------------------------------------
	// KZ Local CS2
	// -----------------------------------------------------------------------

	/** GET /local/cs2kz/maps (limit/offset/sort) */
	virtual bool GetLocalCS2Maps(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /local/cs2kz/records (limit/offset/sort) */
	virtual bool GetLocalCS2Records(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /local/cs2kz/players (limit/offset/sort) */
	virtual bool GetLocalCS2Players(FKZ_ResponseCallback callback, int limit = 0, int offset = 0, const char *sort = "", void *data = nullptr) = 0;

	/** GET /local/cs2kz/stats */
	virtual bool GetLocalCS2Stats(FKZ_ResponseCallback callback, void *data = nullptr) = 0;
};

#endif // _INCLUDE_IFKZAPI_H_
