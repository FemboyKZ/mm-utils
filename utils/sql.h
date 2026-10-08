#ifndef _INCLUDE_MMU_SQL_H_
#define _INCLUDE_MMU_SQL_H_

#include <cstdint>
#include <deque>
#include <functional>
#include <string>
#include <utility>

// Forward declarations from sql_mm.
class ISQLInterface;
class ISQLConnection;
class ISQLQuery;
class IMySQLClient;
class ISQLiteClient;

namespace mmu
{
	namespace sql
	{
		enum class DbType
		{
			MySQL,
			SQLite
		};

		// Connection details passed to Connect. Only the fields for the active DbType are read.
		// Copied into the connection so they outlive the async connect.
		struct ConnectParams
		{
			// SQLite: path relative to the game dir (e.g. game/csgo/).
			std::string path;
			// MySQL connection.
			std::string host;
			std::string user;
			std::string pass;
			std::string database;
			int port = 3306;
		};

		// sql_mm-backed async connection.
		// Callbacks fire on the main thread so they may touch game state directly.
		class Connection
		{
		public:
			~Connection();

			// Acquire ISQLInterface via MetaFactory and select the client for `type`.
			// Call in AllPluginsLoaded or later (sql_mm must load first).
			// Returns false if sql_mm or the client is unavailable.
			bool Init(DbType type);

			// Run after a successful connect and pragmas, before the connect callback.
			// The consumer creates its schema here (via Query).
			// Set once, before Connect.
			void SetSchemaHook(std::function<void()> hook);

			// Async connect using `params`.
			// On success runs the standard pragmas, invokes the schema hook, then fires cb(true).
			// cb fires on the main thread.
			// No-op firing cb(false) if a connect is already in flight.
			// Any previous connection is destroyed first.
			// A connect that fails fires cb(false) once. RunFrame keeps trying after that, and fires cb(true) if a try gets through.
			void Connect(const ConnectParams &params, std::function<void(bool)> cb);

			// Call every frame to have a failed connect retried, `now` in seconds on any clock that only runs forward.
			void RunFrame(double now);

			// Destroy the connection and latch shutdown so in-flight callbacks bail.
			void Shutdown();

			bool IsConnected() const
			{
				return m_connected;
			}

			// True while an async connect is in flight.
			// Retry timers must check this. IsConnected() alone is false during the initial connect,
			// so a timer gated only on that will fire mid-connect.
			bool IsConnecting() const
			{
				return m_connecting;
			}

			bool IsInitialized() const
			{
				return m_initialized;
			}

			// True from the moment Shutdown() is called.
			// Guards against in-flight callbacks landing after unload begins.
			bool IsShuttingDown() const
			{
				return m_shuttingDown;
			}

			DbType Type() const
			{
				return m_type;
			}

			bool IsSQLite() const
			{
				return m_type == DbType::SQLite;
			}

			bool IsMySQL() const
			{
				return m_type == DbType::MySQL;
			}

			// Raw connection for direct queries. Null until connected.
			ISQLConnection *Raw() const
			{
				return m_conn;
			}

			// Run a query. cb(nullptr) if not connected or the query failed. cb fires on the main thread.
			// sql_mm says nothing about a failed query, so its cb(nullptr) only comes once a later query answers.
			void Query(const char *query, std::function<void(ISQLQuery *)> cb);

			// printf-style query.
			void QueryFmt(std::function<void(ISQLQuery *)> cb, const char *fmt, ...);

			// Escape a string for SQL. Quotes are doubled by hand when not connected.
			std::string Escape(const char *str);

		private:
			struct PendingQuery
			{
				uint64_t id;
				std::function<void(ISQLQuery *)> cb;
			};

			void Open(const ConnectParams &params, std::function<void(bool)> cb, bool retry);
			void Answer(uint64_t id, ISQLQuery *result);

			// Queries with a callback that sql_mm has not answered yet, oldest first.
			std::deque<PendingQuery> m_pending;
			uint64_t m_lastQueryId = 0;

			std::function<void(bool)> m_connectCb;
			// RunFrame's last `now`, the connect callback has no clock of its own.
			double m_frameTime = 0.0;
			// 0 while no retry is due.
			double m_retryAt = 0.0;

			ISQLInterface *m_sql = nullptr;
			IMySQLClient *m_mysql = nullptr;
			ISQLiteClient *m_sqlite = nullptr;
			ISQLConnection *m_conn = nullptr;
			std::function<void()> m_schemaHook;
			ConnectParams m_params;
			DbType m_type = DbType::SQLite;
			bool m_connected = false;
			bool m_connecting = false;
			bool m_initialized = false;
			bool m_shuttingDown = false;
		};

		// SQL fragment matching a Steam authid by suffix in both MySQL and SQLite:
		//   (<column> LIKE 'STEAM_0:<suffix>' OR <column> LIKE 'STEAM_1:<suffix>')
		// `escapedSuffix` is the already-escaped "X:Y" portion.
		std::string AuthMatch(const char *column, const std::string &escapedSuffix);

	} // namespace sql
} // namespace mmu

#endif // _INCLUDE_MMU_SQL_H_
