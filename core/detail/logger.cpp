/*
 * Created by switchblade on 03/05/22
 */

#include "../logger.hpp"

#include <cstdio>

namespace sek
{
	template<typename L, typename T>
	struct instance_init
	{
		static void log_message(const typename L::string_type &msg) { fmt::print("{}\n", msg); }

		constexpr instance_init(const auto &level, bool enable = true) : m_data(level)
		{
			m_data.logger.on_log() += delegate_func<log_message>;
			if (!enable) m_data.logger.disable();
		}

		constexpr operator shared_guard<L *> () noexcept { return {&m_data.logger, &m_data.mtx}; }

	private:
		T m_data;
	};

	template<>
	shared_guard<logger *> logger::info()
	{
		static auto instance = instance_init<logger, guarded_instance>{"INFO"};
		return instance;
	}
	template<>
	shared_guard<logger *> logger::warn()
	{
		static auto instance = instance_init<logger, guarded_instance>{"WARN"};
		return instance;
	}
	template<>
	shared_guard<logger *> logger::debug()
	{
#ifndef SEK_DEBUG
		static auto instance = instance_init<logger, guarded_instance>{"DEBUG", false};
#else
		static auto instance = instance_init<logger, guarded_instance>{"DEBUG"};
#endif
		return instance;
	}
	template<>
	shared_guard<logger *> logger::error()
	{
		static auto instance = instance_init<logger, guarded_instance>{"ERROR"};
		return instance;
	}
	template<>
	shared_guard<logger *> logger::fatal()
	{
		static auto instance = instance_init<logger, guarded_instance>{"FATAL"};
		return instance;
	}

	template class SEK_API_EXPORT basic_logger<char>;
}	 // namespace sek