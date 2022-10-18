/*
 * Created by switchblade on 12/05/22
 */

#pragma once

#include <filesystem>
#include <vector>

#include "access_guard.hpp"
#include "expected.hpp"
#include "type_name.hpp"
#include "version.hpp"

namespace sek
{
	class plugin_interface;
	template<typename>
	class plugin_group;
	template<template_instance<plugin_group>>
	class plugin_ptr;
	template<basic_static_string, version, template_instance<plugin_group>>
	class plugin;

	namespace detail
	{
		using module_path_char = typename std::filesystem::path::value_type;
		using module_handle = void *;

		struct module_data;

		template<typename P>
		extern SEK_API_IMPORT P plugin_instance;
	}	 // namespace detail

	/** @brief Structure used to implement common base functionality for all plugin interfaces. */
	class SEK_CORE_PUBLIC plugin_interface
	{
		template<basic_static_string, version, template_instance<plugin_group>>
		friend class plugin;
		template<template_instance<plugin_group>>
		friend class plugin_ptr;

	public:
		plugin_interface() noexcept = default;
		virtual ~plugin_interface() = 0;

		/** Checks if the plugin is enabled. */
		[[nodiscard]] constexpr bool is_enabled() const noexcept { return m_enabled; }
		/** Returns the version of `sekhmet-core` the plugin was built for. */
		[[nodiscard]] constexpr version core_ver() const noexcept { return m_core_ver; }
		/** Returns the version of `sekhmet-core` the plugin was built for. */
		[[nodiscard]] constexpr version plugin_ver() const noexcept { return m_core_ver; }
		/** Returns the display name of the plugin. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }

		/** Function invoked when a plugin is enabled. */
		virtual void enable();
		/** Function invoked when a plugin is disabled.
		 * @note Automatically destroys all plugin-local objets. */
		virtual void disable();

	private:
		/* Mutex used to reference the plugin via a `plugin_ptr`. */
		std::recursive_mutex m_mtx;
		bool m_enabled = false;

		version m_core_ver = SEK_CORE_VERSION;
		version m_plugin_ver;

		std::string_view m_name;
	};

	/** @brief Pointer-like structure used to reference an instance of a plugin.
	 * @tparam Group An instance of `plugin_group`, used to define a group the referenced plugin belongs to. */
	template<template_instance<plugin_group> Group>
	class plugin_ptr
	{
		friend class module;
		template<typename>
		friend class plugin_group;

	public:
		typedef typename Group::interface_type element_type;
		typedef typename Group::interface_type interface_type;

	private:
		constexpr explicit plugin_ptr(const interface_type *ptr) noexcept : m_ptr(ptr) {}

	public:
		/** Initializes an empty plugin pointer. */
		constexpr plugin_ptr() noexcept = default;

		/** Checks if the plugin pointer references a plugin instance. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_ptr == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns an access guard to the referenced plugin instance. */
		[[nodiscard]] constexpr access_guard<interface_type, std::recursive_mutex> get() const noexcept
		{
			return {static_cast<interface_type *>(m_ptr), &m_ptr->m_mtx};
		}
		/** @copydoc get */
		[[nodiscard]] constexpr access_guard<interface_type, std::recursive_mutex> operator->() const noexcept
		{
			return get();
		}

		// clang-format off
		[[nodiscard]] constexpr auto operator<=>(const plugin_ptr &other) const noexcept { return get() <=> other.get(); }
		[[nodiscard]] constexpr bool operator==(const plugin_ptr &other) const noexcept { return get() == other.get(); }
		// clang-format on

		constexpr void swap(plugin_ptr &other) noexcept { std::swap(m_ptr, other.m_ptr); }
		friend constexpr void swap(plugin_ptr &a, plugin_ptr &b) noexcept { a.swap(b); }

	private:
		const interface_type *m_ptr = nullptr;
	};

	/** @brief Base type used to implement plugins.
	 * @tparam Group An instance of `plugin_group`, used to define a group this plugin belongs to.
	 * @tparam Name Display name of the plugin.
	 * @tparam Version Version of the plugin.
	 * @note Child plugin types must publicly inherit from `plugin`.
	 * @note Child plugin types must be instantiated via `SEK_PLUGIN_INSTANCE`. */
	template<basic_static_string Name, version Version, template_instance<plugin_group> Group>
	class plugin : public Group::interface_type
	{
	public:
		typedef Group group_type;

	public:
		plugin();
		virtual ~plugin();
	};

	/** @brief Structure used to implement and interface with plugin groups.
	 * @tparam Pb Base interface for plugins of this group.
	 * @note Plugin interface must be a specialization of `plugin_interface`. */
	template<typename I>
	class plugin_group
	{
		static_assert(std::is_base_of_v<plugin_interface, I>, "Plugin interface must be a specialization of `plugin_interface`");

	public:
		typedef I interface_type;

	public:
		/* TODO: Implement a static function to get plugin instances of this group for all modules. */
		/* TODO: Implement a static function to invoke an interface member function for every instance. */
	};

	template<typename T>
	struct type_name<plugin_group<T>>
	{
	private:
		[[nodiscard]] constexpr static auto format() noexcept
		{
			return basic_static_string{"sek::plugin_group<"} + type_name_v<T> + basic_static_string{">"};
		}

	public:
		constexpr static std::string_view value = format();
	};

	/** @brief Handle used to reference a native dynamic library which contains one or multiple plugins. */
	class module
	{
		template<basic_static_string, version, template_instance<plugin_group>>
		friend class plugin;

	public:
		typedef detail::module_handle native_handle_type;

	private:
		using path_char = detail::module_path_char;
		using data_t = detail::module_data;

	public:
		/** Returns module handle referencing the main (parent executable) module. */
		[[nodiscard]] static SEK_CORE_PUBLIC module main();
		/** Returns a vector of module handles to all currently opened modules. */
		[[nodiscard]] static SEK_CORE_PUBLIC std::vector<module> all();

	private:
		static SEK_CORE_PUBLIC void register_plugin(std::string_view group, plugin_interface *ptr) noexcept;
		static SEK_CORE_PUBLIC void unregister_plugin(std::string_view group, plugin_interface *ptr) noexcept;

		template<typename T>
		inline static T return_if(expected<T, std::error_code> &&exp)
		{
			if (!exp.has_value()) [[unlikely]]
				throw std::system_error(exp.error());

			if constexpr (!std::is_void_v<T>) return std::move(exp.value());
		}

		constexpr explicit module(data_t *data) noexcept : m_data(data) {}

	public:
		/** Initializes an empty module handle. */
		constexpr module() noexcept = default;
		~module() { unload(); }

		SEK_CORE_PUBLIC module(const module &);
		SEK_CORE_PUBLIC module &operator=(const module &);

		constexpr module(module &&other) noexcept { swap(other); }
		constexpr module &operator=(module &&other) noexcept
		{
			swap(other);
			return *this;
		}

		/** Loads a module.
		 * @param path Path to the module library.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once.
		 * @note If the module path is `nullptr`, initializes a module handle to the main (parent executable) module. */
		explicit module(const path_char *path) { load(path); }
		/** @copydoc native_file */
		explicit module(const std::filesystem::path &path) : module(path.c_str()) {}

		/** @brief Loads a module.
		 * @param path Path to the module library.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once. */
		void load(const std::filesystem::path &path) { return load(path.c_str()); }
		/** @copydoc open
		 * @note If the module path is `nullptr`, opens a module handle to the main (parent executable) module. */
		SEK_CORE_PUBLIC void load(const path_char *path);
		/** @copybrief open
		 * @param path Path to the module library.
		 * @return `void` or an error code.
		 * @note A module library is guaranteed to be loaded only once. */
		expected<void, std::error_code> load(std::nothrow_t, const std::filesystem::path &path)
		{
			return load(std::nothrow, path.c_str());
		}
		/** @copydoc open
		 * @note If the module path is `nullptr`, opens a module handle to the main (parent executable) module. */
		SEK_CORE_PUBLIC expected<void, std::error_code> load(std::nothrow_t, const path_char *path);

		/** @brief Unloads a non-empty module handle. If the internal reference counter reaches 0, unloads the module.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note Does nothing if the module handle is empty or references the main (parent executable) module. */
		SEK_CORE_PUBLIC void unload();
		/** @copybrief unload
		 * @return `void` or an error code.
		 * @note Does nothing if the module handle is empty or references the main (parent executable) module. */
		SEK_CORE_PUBLIC expected<void, std::error_code> unload(std::nothrow_t);

		/** Checks if the module handle references a loaded module. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_data == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/* TODO: Implement a function to get plugin instances for a specific group. */

		/** Returns the underlying native module handle. */
		[[nodiscard]] SEK_CORE_PUBLIC native_handle_type native_handle() const noexcept;
		/** Returns absolute path to the underlying module library. */
		[[nodiscard]] SEK_CORE_PUBLIC const std::filesystem::path &path() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const module &other) const noexcept { return m_data <=> other.m_data; }
		[[nodiscard]] constexpr bool operator==(const module &other) const noexcept { return m_data == other.m_data; }

		constexpr void swap(module &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(module &a, module &b) noexcept { a.swap(b); }

	protected:
		void copy_init(const module &other);

		data_t *m_data = nullptr;
	};

	template<basic_static_string N, version V, template_instance<plugin_group> G>
	plugin<N, V, G>::plugin()
	{
		plugin_interface::m_name = static_string_cast<char>(N);
		plugin_interface::m_plugin_ver = V;

		/* TODO: Register plugin instance. */
	}
	template<basic_static_string N, version V, template_instance<plugin_group> G>
	plugin<N, V, G>::~plugin()
	{
		/* TODO: Unregister plugin instance. */
	}

	/** @brief Plugin interface used for the core plugin group. */
	class SEK_CORE_PUBLIC core_plugin_interface : public plugin_interface
	{
	public:
		core_plugin_interface() noexcept = default;
		virtual ~core_plugin_interface() override = 0;
	};

	template<>
	struct type_name<plugin_group<core_plugin_interface>>
	{
		constexpr static std::string_view value = "sek::core_plugin_group";
	};
	template<>
	struct type_name<core_plugin_interface>
	{
		constexpr static std::string_view value = "sek::core_plugin_interface";
	};

	extern template class SEK_API_IMPORT plugin_group<core_plugin_interface>;

	/** @brief Core plugin group type (alias for `plugin_group<core_plugin_interface>`). */
	using core_plugin_group = plugin_group<core_plugin_interface>;
	/** @brief Core plugin base type (alias for `plugin<core_plugin_group, Name>`).
	 * @tparam Name Display name of the plugin.
	 * @tparam Version Version of the plugin. */
	template<basic_static_string Name, version Version>
	using core_plugin = plugin<Name, Version, core_plugin_group>;
}	 // namespace sek

// clang-format off
/** @brief Macro used to instantiate & register a plugin of type `T` during static initialization of a module.
 * Any additional arguments are passed to the constructor ot `T`.
 *
 * @example
 * @code{cpp}
 * // my_core_plugin.hpp
 * struct my_core_plugin : sek::core_plugin<"My Core Plugin", "1.0.0">
 * {
 * 	my_core_plugin(int i);
 *
 * 	// Function called when a plugin is enabled.
 * 	void enable() override;
 * 	// Function called when a plugin is disabled.
 * 	void disable() override;
 * };
 *
 * // my_core_plugin.cpp
 * SEK_PLUGIN_INSTANCE(my_core_plugin, 1)
 * @endcode */
#define SEK_PLUGIN_INSTANCE(T, ...) template<> SEK_API_EXPORT T sek::detail::plugin_instance<T> = T(__VA_ARGS__);
// clang-format on