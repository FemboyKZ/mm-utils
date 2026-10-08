#include "utils/sql.h"
#include "utils/log.h"

#include "interfaces/sql_mm/sql_mm.h"
#include "interfaces/sql_mm/mysql_mm.h"
#include "interfaces/sql_mm/sqlite_mm.h"

#include <ISmmPlugin.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

// Provided by the consuming plugin.
extern ISmmAPI *g_SMAPI;

namespace mmu
{
	namespace sql
	{
		constexpr double kRetrySeconds = 30.0;

		Connection::~Connection()
		{
			Shutdown();
		}

		bool Connection::Init(DbType type)
		{
			m_sql = static_cast<ISQLInterface *>(g_SMAPI->MetaFactory(SQLMM_INTERFACE, nullptr, nullptr));
			if (!m_sql)
			{
				MMU_LOG_WARN("Failed to get ISQLInterface. Is sql_mm loaded?\n");
				return false;
			}

			m_type = type;

			if (type == DbType::SQLite)
			{
				m_sqlite = m_sql->GetSQLiteClient();
				if (!m_sqlite)
				{
					MMU_LOG_WARN("Failed to get SQLite client from sql_mm.\n");
					return false;
				}
				MMU_LOG_INFO("Database type: SQLite.\n");
			}
			else
			{
				m_mysql = m_sql->GetMySQLClient();
				if (!m_mysql)
				{
					MMU_LOG_WARN("Failed to get MySQL client from sql_mm.\n");
					return false;
				}
				MMU_LOG_INFO("Database type: MySQL.\n");
			}

			m_initialized = true;
			return true;
		}

		void Connection::SetSchemaHook(std::function<void()> hook)
		{
			m_schemaHook = std::move(hook);
		}

		void Connection::Connect(const ConnectParams &params, std::function<void(bool)> cb)
		{
			Open(params, std::move(cb), false);
		}

		void Connection::RunFrame(double now)
		{
			m_frameTime = now;
			if (m_retryAt <= 0.0 || now < m_retryAt)
			{
				return;
			}
			// Copies, Open assigns to both members.
			Open(ConnectParams(m_params), std::function<void(bool)>(m_connectCb), true);
		}

		void Connection::Open(const ConnectParams &params, std::function<void(bool)> cb, bool retry)
		{
			// A second Connect while one is in flight would orphan the pending connection.
			if (m_connecting)
			{
				MMU_LOG_WARN("Connect called while a connect is already in flight. Ignoring.\n");
				if (cb)
				{
					cb(false);
				}
				return;
			}

			m_retryAt = 0.0;
			m_connectCb = cb;

			// Destroy the old connection before m_params is reassigned.
			// MySQLConnectionInfo holds raw pointers into the m_params strings,
			// so it must not outlive them.
			if (m_conn)
			{
				m_conn->Destroy();
				m_conn = nullptr;
				m_connected = false;
				m_pending.clear();
			}

			m_params = params;

			if (m_type == DbType::SQLite)
			{
				if (!m_sqlite)
				{
					MMU_LOG_WARN("Cannot connect: SQLite client not initialized.\n");
					if (cb)
					{
						cb(false);
					}
					return;
				}

				SQLiteConnectionInfo info;
				info.database = m_params.path.c_str();
				m_conn = m_sqlite->CreateSQLiteConnection(info);
			}
			else
			{
				if (!m_mysql)
				{
					MMU_LOG_WARN("Cannot connect: MySQL client not initialized.\n");
					if (cb)
					{
						cb(false);
					}
					return;
				}

				if (m_params.host.empty() || m_params.database.empty())
				{
					MMU_LOG_WARN("Cannot connect: MySQL host or database is empty. Check core.cfg.\n");
					if (cb)
					{
						cb(false);
					}
					return;
				}

				MySQLConnectionInfo info;
				info.host = m_params.host.c_str();
				info.user = m_params.user.c_str();
				info.pass = m_params.pass.c_str();
				info.database = m_params.database.c_str();
				info.port = m_params.port;
				m_conn = m_mysql->CreateMySQLConnection(info);
			}

			if (!m_conn)
			{
				MMU_LOG_WARN("Failed to create database connection object.\n");
				if (cb)
				{
					cb(false);
				}
				return;
			}

			m_connecting = true;
			m_conn->Connect(
				[this, cb, retry](bool success)
				{
					m_connecting = false;
					m_connected = success;
					if (success)
					{
						MMU_LOG_INFO("Database connected (%s).\n", IsSQLite() ? "SQLite" : "MySQL");
						if (IsSQLite())
						{
							Query("PRAGMA journal_mode=WAL", [](ISQLQuery *) {});
							Query("PRAGMA foreign_keys=ON", [](ISQLQuery *) {});
						}
						else
						{
							Query("SET NAMES utf8mb4", [](ISQLQuery *) {});
						}
						if (m_schemaHook)
						{
							m_schemaHook();
						}
					}
					else
					{
						// Said once, sql_mm prints its own line for every attempt.
						if (!retry)
						{
							MMU_LOG_WARN("Database connection failed.\n");
						}
						m_retryAt = m_frameTime + kRetrySeconds;
					}
					// The consumer already heard that it failed.
					if (cb && (success || !retry))
					{
						cb(success);
					}
				});
		}

		void Connection::Shutdown()
		{
			m_shuttingDown = true;
			if (m_conn)
			{
				m_conn->Destroy();
				m_conn = nullptr;
			}
			m_pending.clear();
			m_retryAt = 0.0;
			m_connected = false;
			m_connecting = false;
			m_initialized = false;
			m_mysql = nullptr;
			m_sqlite = nullptr;
			m_sql = nullptr;
		}

		void Connection::Query(const char *query, std::function<void(ISQLQuery *)> cb)
		{
			if (m_shuttingDown || !m_conn || !m_connected)
			{
				if (cb)
				{
					cb(nullptr);
				}
				return;
			}
			// sql_mm's const char * overload is printf-style and would read every % in the query, escaped player text included,
			// as a format specifier. The char * one sends it as is, and copies it before returning.
			char *sql = const_cast<char *>(query);

			// sql_mm requires a valid callback, never pass a null std::function.
			if (!cb)
			{
				m_conn->Query(sql, [](ISQLQuery *) {});
				return;
			}

			const uint64_t id = ++m_lastQueryId;
			m_pending.push_back({id, std::move(cb)});
			m_conn->Query(sql, [this, id](ISQLQuery *result) { Answer(id, result); });
		}

		void Connection::Answer(uint64_t id, ISQLQuery *result)
		{
			// sql_mm answers in the order queries were sent and skips one that failed,
			// so whatever was sent before this one and is still waiting has failed.
			// Checked again each round, a callback can send queries or drop the connection.
			while (!m_pending.empty() && m_pending.front().id <= id)
			{
				PendingQuery pending = std::move(m_pending.front());
				m_pending.pop_front();
				pending.cb(pending.id == id ? result : nullptr);
			}
		}

		void Connection::QueryFmt(std::function<void(ISQLQuery *)> cb, const char *fmt, ...)
		{
			va_list args;
			va_start(args, fmt);
			va_list sizing;
			va_copy(sizing, args);
			const int length = vsnprintf(nullptr, 0, fmt, sizing);
			va_end(sizing);

			// Sized to fit: a query cut short would still be sent.
			std::string query(length > 0 ? length : 0, '\0');
			vsnprintf(query.data(), query.size() + 1, fmt, args);
			va_end(args);
			Query(query.c_str(), cb);
		}

		std::string Connection::Escape(const char *str)
		{
			if (!str)
			{
				str = "";
			}

			if (m_conn && m_connected)
			{
				// sql_mm's SQLite escaper hands back its whole work buffer, a NUL and padding after the text.
				// Left in, they cut a query built by appending short and fail AuthMatch.
				std::string escaped = m_conn->Escape(str);
				escaped.resize(strlen(escaped.c_str()));
				return escaped;
			}

			// The driver's escaper needs a live connection.
			// A doubled quote reads as a literal quote on both backends. MySQL also treats backslash as an escape, so it gets doubled there.
			std::string out;
			for (const char *p = str; *p; p++)
			{
				if (*p == '\'')
				{
					out += "''";
				}
				else if (*p == '\\' && IsMySQL())
				{
					out += "\\\\";
				}
				else
				{
					out += *p;
				}
			}
			return out;
		}

		std::string AuthMatch(const char *column, const std::string &escapedSuffix)
		{
			// The suffix lands inside a LIKE pattern, where an unfiltered % or _ would match other players' IDs.
			// Nothing but "Y:Z" is a real suffix, so anything else matches nobody rather than everybody.
			bool valid = !escapedSuffix.empty();
			int colons = 0;
			for (char c : escapedSuffix)
			{
				if (c == ':')
				{
					colons++;
				}
				else if (c < '0' || c > '9')
				{
					valid = false;
					break;
				}
			}
			if (!valid || colons != 1 || escapedSuffix.front() == ':' || escapedSuffix.back() == ':')
			{
				return "0 = 1";
			}

			char buf[256];
			snprintf(buf, sizeof(buf), "(%s LIKE 'STEAM_0:%s' OR %s LIKE 'STEAM_1:%s')", column, escapedSuffix.c_str(), column,
					 escapedSuffix.c_str());
			return std::string(buf);
		}

	} // namespace sql
} // namespace mmu
