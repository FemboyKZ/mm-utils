#include "mmu/sql.h"
#include "mmu/log.h"

#include <sql_mm.h>
#include <mysql_mm.h>
#include <sqlite_mm.h>

#include <ISmmAPI.h>

#include <cstdarg>
#include <cstdio>

// Provided by the consuming plugin.
extern ISmmAPI *g_SMAPI;

namespace mmu
{
	namespace sql
	{
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

			m_conn->Connect(
				[this, cb](bool success)
				{
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
						MMU_LOG_WARN("Database connection failed.\n");
					}
					if (cb)
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
			m_connected = false;
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
			// sql_mm requires a valid callback, never pass a null std::function.
			m_conn->Query(query, cb ? cb : [](ISQLQuery *) {});
		}

		void Connection::QueryFmt(std::function<void(ISQLQuery *)> cb, const char *fmt, ...)
		{
			char buffer[4096];
			va_list args;
			va_start(args, fmt);
			vsnprintf(buffer, sizeof(buffer), fmt, args);
			va_end(args);
			Query(buffer, cb);
		}

		std::string Connection::Escape(const char *str)
		{
			if (!m_conn)
			{
				return str ? str : "";
			}
			return m_conn->Escape(str);
		}

		std::string AuthMatch(const char *column, const std::string &escapedSuffix)
		{
			char buf[256];
			snprintf(buf, sizeof(buf), "(%s LIKE 'STEAM_0:%s' OR %s LIKE 'STEAM_1:%s')", column, escapedSuffix.c_str(), column,
					 escapedSuffix.c_str());
			return std::string(buf);
		}

	} // namespace sql
} // namespace mmu
