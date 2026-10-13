#ifndef _INCLUDE_MMU_CONFIG_BLOCKS_H_
#define _INCLUDE_MMU_CONFIG_BLOCKS_H_

#include "utils/log.h"
#include "utils/sql.h"
#include "utils/str.h"

#include <cctype>
#include <cstdlib>
#include <string>

namespace mmu
{
	namespace config
	{
		// No ChatPrefix here, its color tags need chat_colors.cpp, which console-only plugins don't build.
		struct GeneralBlock
		{
			std::string commandPrefix = "!";
			std::string silentCommandPrefix = "/";
			std::string defaultLanguage = "en";
		};

		// `key` must be lowercase. Returns false for keys it does not own.
		inline bool ApplyGeneralKey(GeneralBlock &g, const std::string &key, const std::string &value)
		{
			if (key == "commandprefix")
			{
				g.commandPrefix = value;
			}
			else if (key == "silentcommandprefix")
			{
				g.silentCommandPrefix = value;
			}
			else if (key == "defaultlanguage")
			{
				g.defaultLanguage = value;
			}
			else
			{
				return false;
			}
			return true;
		}

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
				// It goes into table names as is.
				db.prefix.clear();
				for (char c : value)
				{
					if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
					{
						db.prefix += c;
					}
				}
				if (db.prefix != value)
				{
					MMU_LOG_WARN("Database prefix '%s' may only hold letters, digits and underscores, using '%s'.\n", value.c_str(),
								 db.prefix.c_str());
				}
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

		// Also takes "enabled", for an optional database.
		inline bool ApplyOptionalDatabaseKey(DatabaseBlock &db, bool &enabled, const std::string &key, const std::string &value)
		{
			if (key == "enabled")
			{
				enabled = (value != "0");
				return true;
			}
			return ApplyDatabaseKey(db, key, value);
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
