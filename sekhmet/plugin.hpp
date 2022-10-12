/*
 * Created by switchblade on 12/05/22
 */

#pragma once

#include <vector>

#include "../event.hpp"
#include "../static_string.hpp"
#include "../version.hpp"

namespace sek
{
	namespace detail
	{
		struct plugin_info
		{
			consteval plugin_info(version core_ver, version plugin_ver, std::string_view id) noexcept
				: core_ver(core_ver), plugin_ver(plugin_ver), id(id)
			{
			}

			/** Version of the core framework the plugin was compiled for. */
			const version core_ver;
			/** Version of the plugin. */
			const version plugin_ver;
			/** Id of the plugin. */
			const std::string_view id;
		};
		struct plugin_data
		{
			enum status_t
			{
				INITIAL,
				DISABLED,
				ENABLED,
			};

			SEK_CORE_PUBLIC static void load(plugin_data *data, void (*init)(void *));
			static void load_impl(plugin_data *data, void (*init)(void *));

			SEK_CORE_PUBLIC static void unload(plugin_data *data);
			static void unload_impl(plugin_data *data);

			explicit plugin_data(plugin_info info) noexcept : info(info) {}

			[[nodiscard]] bool enable() const
			{
				bool result;
				on_enable.dispatch([&result](bool b) { return (result = b); });
				return result;
			}
			void disable() const { on_disable(); }

			/** Compile-time information about this plugin. */
			const plugin_info info;
			/** Event dispatched when a plugin is enabled. */
			event<bool(void)> on_enable;
			/** Event dispatched when a plugin is disabled. */
			event<void(void)> on_disable;

			status_t status;
		};

		template<typename Child>
		class plugin_base : plugin_data
		{
		public:
			static Child &instance();

			explicit plugin_base(plugin_info info) noexcept : plugin_data(info)
			{
				plugin_data::load(this, [](void *p) { static_cast<Child *>(p)->init(); });
			}

			using plugin_data::info;
			using plugin_data::on_disable;
			using plugin_data::on_enable;
		};
	}	 // namespace detail

	using modmode = int;
	constexpr modmode lazy = 1;
	constexpr modmode deep = 2;
	constexpr modmode global = 4;
	constexpr modmode nounload = 8;

	/** @brief Handle used to reference a native dynamic library which contains one or multiple plugins. */
	class module
	{

	public:
		typedef void *native_handle_type;
		typedef modmode modmode;

		constexpr static modmode lazy = modmode;
		constexpr static modmode deep = modmode;
		constexpr static modmode global = modmode;
		constexpr static modmode nounload = modmode;

		/** Returns a module handle to the main (parent executable) module. */
		[[nodiscard]] static SEK_CORE_PUBLIC module main() noexcept;

	private:
		struct module_data;

		using path_char = typename std::filepath::value_type;

		template<typename T>
		inline static T return_if(expected<T, std::error_code> &&exp)
		{
			if (!exp.has_value()) [[unlikely]]
				throw std::system_error(exp.error());

			if constexpr (!std::is_void_v<T>) return std::move(exp.value());
		}

	public:
		module(const module &) = delete;
		module &operator=(const module &) = delete;

		/** Initializes an invalid (empty) module. */
		constexpr module() noexcept = default;
		SEK_CORE_PUBLIC ~module();

		constexpr module(module &&other) noexcept { swap(other); }
		constexpr module &operator=(module &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Loads & initializes a module.
		 * @param path Path to the module library.
		 * @param mode Mode to load the module with.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once. */
		module(const path_char *path, modmode mode) { open(path, mode); }
		/** @copydoc native_file */
		module(const std::filepath &path, openmode mode) :module(path.c_str(), mode) {}

		/** @brief Loads a module.
		 * @param path Path to the module library.
		 * @param mode Mode to load the module with.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once. */
		void open(const std::filepath &path, modmode mode) { open(path.c_str(), mode); }
		/** @copydoc open */
		SEK_CORE_PUBLIC void open(const path_char *path, modmode mode);

		/** @copybrief open
		 * @param path Path to the module library.
		 * @param mode Mode to load the module with.
		 * @return `void` or an error code.
		 * @note A module library is guaranteed to be loaded only once. */
		expected<void, std::error_code> open(std::nothrow_t, const std::filepath &path, modmode mode) noexcept
		{
			return open(std::nothrow, path.c_str(), mode);
		}
		/** @copydoc open */
		SEK_CORE_PUBLIC expected<void, std::error_code> open(std::nothrow_t, const path_char *path, modmode mode) noexcept;

		/** @brief Decrements the internal reference count and unloads the module.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note If the module is loaded in `nounload` mode, it is never unloaded. */
		SEK_CORE_PUBLIC void close();
		/** @copybrief close
		 * @return `void` or an error code.
		 * @note If the module is loaded in `nounload` mode, it is never unloaded */
		SEK_CORE_PUBLIC expected<void, std::error_code> close(std::nothrow_t) noexcept;

		/** Checks if the module handle is empty (does not represent a valid module). */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_data == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns the underlying native library handle. */
		[[nodiscard]] SEK_CORE_PUBLIC native_handle_type native_handle() const noexcept;

		constexpr void swap(native_file &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(native_file &a, native_file &b) noexcept { a.swap(b); }

	protected:
		const module_data *m_data = nullptr;
	};

	/** @brief Handle used to reference and manage plugins. */
	class plugin
	{
	public:
		/** Returns a vector of all currently loaded plugins. */
		SEK_CORE_PUBLIC static std::vector<plugin> get_loaded();
		/** Returns a vector of all currently enabled plugins. */
		SEK_CORE_PUBLIC static std::vector<plugin> get_enabled();

		/** Returns a plugin using it's id. If such plugin does not exist, returns an empty handle. */
		SEK_CORE_PUBLIC static plugin get(std::string_view id);

	private:
		constexpr explicit plugin(detail::plugin_data *data) noexcept : m_data(data) {}

	public:
		/** Initializes an empty plugin handle. */
		constexpr plugin() noexcept = default;

		/** Checks if the plugin handle is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_data == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns id of the plugin. */
		[[nodiscard]] constexpr std::string_view id() const noexcept { return m_data->info.id; }
		/** Returns core framework version of the plugin. */
		[[nodiscard]] constexpr version core_ver() const noexcept { return m_data->info.core_ver; }

		/** Checks if the plugin is enabled. */
		[[nodiscard]] SEK_CORE_PUBLIC bool enabled() const noexcept;
		/** Enables the plugin and invokes it's `on_enable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to enable if it is already enabled or not loaded or if `on_enable` returned false or threw an exception. */
		[[nodiscard]] SEK_CORE_PUBLIC bool enable() const noexcept;
		/** Disables the plugin and invokes it's `on_disable` member function.
		 * @returns true on success, false otherwise.
		 * @note Plugin will fail to disable if it is not enabled or not loaded. */
		[[nodiscard]] SEK_CORE_PUBLIC bool disable() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const plugin &) const noexcept = default;
		[[nodiscard]] constexpr bool operator==(const plugin &) const noexcept = default;

	private:
		detail::plugin_data *m_data = nullptr;
	};
}	 // namespace sek

namespace impl
{
	template<typename F>
	struct static_exec
	{
		constexpr static_exec() noexcept : static_exec(F{}) {}
		constexpr static_exec(F &&f) noexcept { std::invoke(f); }
	};
	template<typename F>
	static_exec(F &&) -> static_exec<F>;

	template<sek::basic_static_string Id>
	class plugin_instance : sek::detail::plugin_base<plugin_instance<Id>>
	{
		friend class sek::detail::plugin_base<plugin_instance>;

		static static_exec<void (*)()> bootstrap;

	public:
		using sek::detail::plugin_base<plugin_instance<Id>>::instance;
		using sek::detail::plugin_base<plugin_instance<Id>>::on_disable;
		using sek::detail::plugin_base<plugin_instance<Id>>::on_enable;
		using sek::detail::plugin_base<plugin_instance<Id>>::info;

	private:
		plugin_instance();
		void init();
	};

	template<sek::basic_static_string Func, sek::basic_static_string Plugin>
	struct exec_on_plugin_enable : static_exec<exec_on_plugin_enable<Func, Plugin>>
	{
		static exec_on_plugin_enable instance;
		static bool invoke();

		constexpr void operator()() const noexcept { plugin_instance<Plugin>::instance().on_enable += invoke; }
	};
	template<sek::basic_static_string Func, sek::basic_static_string Plugin>
	struct exec_on_plugin_disable : static_exec<exec_on_plugin_disable<Func, Plugin>>
	{
		static exec_on_plugin_disable instance;
		static void invoke();

		constexpr void operator()() const noexcept { plugin_instance<Plugin>::instance().on_disable += invoke; }
	};
}	 // namespace impl

/** @brief Macro used to reference the internal unique type of a plugin.
 * @param id String id used to uniquely identify a plugin. */
#define SEK_PLUGIN(id) impl::plugin_instance<(id)>

/** @brief Macro used to define an instance of a plugin.
 * @param id String id used to uniquely identify a plugin.
 * @param ver Version of the plugin in the following format: `"<major>.<minor>.<patch>"`.
 *
 * @example
 * @code{.cpp}
 * SEK_PLUGIN_INSTANCE("my_plugin", "0.1.2")
 * {
 * 	printf("%s is initializing! version: %d.%d.%d\n",
 * 		   info.id.data(),
 * 		   info.core_ver.major(),
 * 		   info.core_ver.minor(),
 * 		   info.core_ver.patch());
 *
 * 	on_enable += +[]()
 * 	{
 * 		printf("Enabling my_plugin\n");
 * 		return true;
 * 	};
 * 	on_disable += +[]() { printf("Disabling my_plugin\n"); };
 * }
 * @endcode */
#define SEK_PLUGIN_INSTANCE(id, ver)                                                                                   \
	static_assert(SEK_ARRAY_SIZE(id), "Plugin id must not be empty");                                                  \
                                                                                                                       \
	/* Definition of plugin instance constructor. */                                                                   \
	template<>                                                                                                         \
	SEK_PLUGIN(id)::plugin_instance() : plugin_base({sek::version{SEK_CORE_VERSION}, sek::version{ver}, (id)})         \
	{                                                                                                                  \
	}                                                                                                                  \
	/* Static instantiation & bootstrap implementation. 2-stage bootstrap is required in order                         \
	 * to allow for `SEK_PLUGIN(id)::instance()` to be called on static initialization. */                             \
	template<>                                                                                                         \
	impl::plugin_instance<(id)> &sek::detail::plugin_base<impl::plugin_instance<(id)>>::instance()                     \
	{                                                                                                                  \
		static impl::plugin_instance<(id)> value;                                                                      \
		return value;                                                                                                  \
	}                                                                                                                  \
	template<>                                                                                                         \
	impl::static_exec<void (*)()> impl::plugin_instance<(id)>::bootstrap = +[]() { instance(); };                      \
                                                                                                                       \
	/* User-facing initialization function. */                                                                         \
	template<>                                                                                                         \
	void impl::plugin_instance<(id)>::init()

/** @brief Macro used to bootstrap code execution when a plugin is enabled.
 * @param plugin Id of the target plugin.
 * @param func Unique name used to disambiguate functions executed during plugin enable process.
 *
 * @example
 * @code{.cpp}
 * SEK_ON_PLUGIN_ENABLE("test_plugin", "test_enable")
 * {
 * 	printf("test_plugin is enabled\n");
 * 	return true;
 * }
 * @endcode */
#define SEK_ON_PLUGIN_ENABLE(plugin, func)                                                                             \
	template<>                                                                                                         \
	bool impl::exec_on_plugin_enable<(plugin), (func)>::invoke();                                                      \
	template<>                                                                                                         \
	impl::exec_on_plugin_enable<(plugin), (func)> impl::exec_on_plugin_enable<(plugin), (func)>::instance = {};        \
                                                                                                                       \
	template<>                                                                                                         \
	bool impl::exec_on_plugin_enable<(plugin), (func)>::invoke()
/** @brief Macro used to bootstrap code execution when a plugin is disabled.
 * @param plugin Id of the target plugin.
 * @param func Unique name used to disambiguate functions executed during plugin disable process.
 *
 * @example
 * @code{.cpp}
 * SEK_ON_PLUGIN_DISABLE("test_plugin", "test_disable")
 * {
 * 	printf("test_plugin is disabled\n");
 * }
 * @endcode */
#define SEK_ON_PLUGIN_DISABLE(plugin, func)                                                                            \
	template<>                                                                                                         \
	void impl::exec_on_plugin_disable<(plugin), (func)>::invoke();                                                     \
	template<>                                                                                                         \
	impl::exec_on_plugin_disable<(plugin), (func)> impl::exec_on_plugin_disable<(plugin), (func)>::instance = {};      \
                                                                                                                       \
	template<>                                                                                                         \
	void impl::exec_on_plugin_disable<(plugin), (func)>::invoke()

#if defined(SEK_PLUGIN_NAME) && defined(SEK_PLUGIN_VERSION)
/** @brief Macro used to define a plugin with name & version specified by the current plugin project.
 * See `SEK_PLUGIN_INSTANCE` for details. */
#define SEK_PROJECT_PLUGIN_INSTANCE() SEK_PLUGIN_INSTANCE(SEK_PLUGIN_NAME, SEK_PLUGIN_VERSION)
/** @brief Macro used to reference the type of a plugin with name & version specified by the current plugin project.
 * See `SEK_PLUGIN` for details. */
#define SEK_PROJECT_PLUGIN SEK_PLUGIN(SEK_PLUGIN_NAME)
/** @brief Macro used to bootstrap code execution when the current project's plugin is enabled.
 * See `SEK_ON_PLUGIN_ENABLE` for details. */
#define SEK_ON_PROJECT_PLUGIN_ENABLE(func) SEK_ON_PLUGIN_ENABLE(SEK_PLUGIN_NAME, func)
/** @brief Macro used to bootstrap code execution when the current project's plugin is disabled.
 * See `SEK_ON_PLUGIN_DISABLE` for details. */
#define SEK_ON_PROJECT_PLUGIN_DISABLE(func) SEK_ON_PLUGIN_DISABLE(SEK_PLUGIN_NAME, func)
#endif
// clang-format on
