#ifndef _INCLUDE_MMU_CONFIG_BLOCKS_H_
#define _INCLUDE_MMU_CONFIG_BLOCKS_H_

#include "mmu/log.h"
#include "mmu/sql.h"
#include "mmu/str_utils.h"

#include <cstdlib>
#include <string>

namespace mmu
{
	namespace config
	{
		// [Database] connection settings. An "enabled" key, if any, stays with the plugin.
		struct DatabaseBlock
		{
			std::string type = "sqlite"; // "sqlite" or "mysql"
			std::string prefix;          // table-name prefix
			std::string path;            // SQLite: path relative to the game dir
			std::string host = "localhost";
			std::string user = "root";
			std::string pass;
			std::string name;
			int port = 3306;

			static DatabaseBlock Defaults(const char *dbName, const char *sqlitePath, const char *tablePrefix)
			{
				DatabaseBlock b;
				b.name = dbName;
				b.path = sqlitePath;
				b.prefix = tablePrefix;
				return b;
			}

			bool IsMySQL() const
			{
				return type == "mysql";
			}

			sql::DbType DbType() const
			{
				return IsMySQL() ? sql::DbType::MySQL : sql::DbType::SQLite;
			}

			sql::ConnectParams ToConnectParams() const
			{
				sql::ConnectParams p;
				p.path = path;
				p.host = host;
				p.user = user;
				p.pass = pass;
				p.database = name;
				p.port = port;
				return p;
			}
		};

		// `key` must be lowercase. Returns false for keys it does not own.
		// Aliases cover every spelling the plugins accepted before.
		inline bool ApplyDatabaseKey(DatabaseBlock &db, const std::string &key, const std::string &value)
		{
			if (key == "type")
			{
				db.type = str::ToLower(value);
				if (db.type != "sqlite" && db.type != "mysql")
				{
					MMU_LOG_WARN("Unknown database type '%s' (expected sqlite or mysql), using SQLite.\n", value.c_str());
				}
			}
			else if (key == "prefix")
			{
				db.prefix = value;
			}
			else if (key == "path" || key == "db_path")
			{
				db.path = value;
			}
			else if (key == "host")
			{
				db.host = value;
			}
			else if (key == "user" || key == "username")
			{
				db.user = value;
			}
			else if (key == "pass" || key == "password")
			{
				db.pass = value;
			}
			else if (key == "database" || key == "name" || key == "dbname")
			{
				db.name = value;
			}
			else if (key == "port")
			{
				db.port = std::atoi(value.c_str());
			}
			else
			{
				return false;
			}
			return true;
		}

		struct LogBlock
		{
			bool toFile = true;     // mirror log output to addons/<plugin>/logs
			int retentionDays = 30; // delete log files older than this, 0 keeps all
		};

		// `key` must be lowercase. Returns false for keys it does not own.
		inline bool ApplyLogKey(LogBlock &lg, const std::string &key, const std::string &value)
		{
			if (key == "logtofile")
			{
				lg.toFile = (value != "0");
			}
			else if (key == "logretentiondays")
			{
				lg.retentionDays = std::atoi(value.c_str());
			}
			else
			{
				return false;
			}
			return true;
		}

		// Call after each config load.
		inline void ApplyLogBlock(const LogBlock &lg)
		{
			log::SetToFile(lg.toFile);
			log::SetRetentionDays(lg.retentionDays);
		}
	} // namespace config
} // namespace mmu

#endif // _INCLUDE_MMU_CONFIG_BLOCKS_H_
