#ifndef _INCLUDE_MMU_FORWARD_LIST_H_
#define _INCLUDE_MMU_FORWARD_LIST_H_

#include <algorithm>
#include <cstdint>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

namespace mmu
{
	template<typename Fn>
	struct ForwardList
	{
		struct Entry
		{
			uint32_t id;
			int owner;
			Fn fn;
		};

		// 0 for an empty callback, which would end the server when called.
		uint32_t Add(int owner, Fn fn)
		{
			if (!fn)
			{
				return 0;
			}
			uint32_t id = m_nextId++;
			m_entries.push_back({id, owner, std::move(fn)});
			return id;
		}

		void Remove(uint32_t id)
		{
			m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [id](const Entry &e) { return e.id == id; }), m_entries.end());
		}

		void Clear()
		{
			m_entries.clear();
		}

		// Leaks the callbacks: destroying one runs code of the unloaded plugin, already unmapped.
		void Drop(int owner)
		{
			for (Entry &e : m_entries)
			{
				if (e.owner == owner)
				{
					new (&e.fn) Fn();
				}
			}
			m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(), [owner](const Entry &e) { return e.owner == owner; }),
							m_entries.end());
		}

		// Snapshot and copies: a callback may unregister itself or register another.
		template<typename... Args>
		bool Fire(Args... args)
		{
			std::vector<uint32_t> ids;
			ids.reserve(m_entries.size());
			for (const Entry &e : m_entries)
			{
				ids.push_back(e.id);
			}
			for (uint32_t id : ids)
			{
				auto it = std::find_if(m_entries.begin(), m_entries.end(), [id](const Entry &e) { return e.id == id; });
				if (it == m_entries.end())
				{
					continue;
				}
				Fn fn = it->fn;
				if constexpr (std::is_void_v<std::invoke_result_t<Fn &, Args...>>)
				{
					fn(args...);
				}
				else if (fn(args...))
				{
					return true;
				}
			}
			return false;
		}

		std::vector<Entry> m_entries;
		uint32_t m_nextId = 1;
	};
} // namespace mmu

#endif // _INCLUDE_MMU_FORWARD_LIST_H_
