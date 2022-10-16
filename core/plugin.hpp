/*
 * Created by switchblade on 12/05/22
 */

#pragma once

#include <filesystem>
#include <vector>

#include "expected.hpp"
#include "type_name.hpp"

namespace sek
{
	template<typename>
	class plugin_group;
	template<template_instance<plugin_group>>
	class plugin_ptr;
	template<basic_static_string, template_instance<plugin_group>>
	class plugin;

	namespace detail
	{
		using module_path_char = typename std::filesystem::path::value_type;
		using module_handle = void *;
		struct module_data;

		struct plugin_data
		{
			/* Helper used for registration via a macro. */
			template<typename P>
			static P instance;

			template<typename P, typename I = typename P::group_type::interface_type>
			constexpr plugin_data(P *ptr) noexcept : name(ptr->name()), ptr(static_cast<I *>(ptr))
			{
			}

			std::string_view name;
			void *ptr;
		};
	}	 // namespace detail

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
		using data_t = detail::plugin_data;

		constexpr explicit plugin_ptr(const data_t *data) noexcept : m_data(data) {}

	public:
		/** Initializes an empty plugin pointer. */
		constexpr plugin_ptr() noexcept = default;

		/** Checks if the plugin pointer references a plugin instance. */
		[[nodiscard]] constexpr bool empty() const noexcept { return m_data == nullptr; }
		/** @copydoc empty */
		[[nodiscard]] constexpr operator bool() const noexcept { return !empty(); }

		/** Returns the name of the referenced plugin. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_data->name; }

		/** Returns an interface pointer to the referenced plugin instance. */
		[[nodiscard]] constexpr interface_type *get() const noexcept
		{
			return static_cast<interface_type *>(m_data->ptr);
		}
		/** @copydoc get */
		[[nodiscard]] constexpr interface_type *operator->() const noexcept { return get(); }
		/** Returns an interface reference to the referenced plugin instance. */
		[[nodiscard]] constexpr interface_type &operator*() const noexcept { return *get(); }

		// clang-format off
		[[nodiscard]] constexpr auto operator<=>(const plugin_ptr &other) const noexcept { return get() <=> other.get(); }
		[[nodiscard]] constexpr bool operator==(const plugin_ptr &other) const noexcept { return get() == other.get(); }
		// clang-format on

		constexpr void swap(plugin_ptr &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(plugin_ptr &a, plugin_ptr &b) noexcept { a.swap(b); }

	private:
		const data_t *m_data = nullptr;
	};

	/** @brief Base type used to implement plugins.
	 * @tparam Group An instance of `plugin_group`, used to define a group this plugin belongs to.
	 * @tparam Name Display name of the plugin.
	 * @note Child plugin types must publicly inherit from `plugin`.
	 * @note Child plugin types must be instantiated via `SEK_PLUGIN_INSTANCE`,
	 * or via manually creating a static instance within a module. */
	template<basic_static_string Name, template_instance<plugin_group> Group>
	class plugin : public Group::interface_type
	{
		static_assert(std::same_as<typename std::remove_cvref_t<decltype(Name)>::value_type, char>,
					  "Plugin name must be a string of `char`");

		using base_t = typename Group::interface_type;

	public:
		typedef Group group_type;

	public:
		plugin();
		virtual ~plugin();

		/** Returns the name of the plugin. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return Name; };
	};

	/** @brief Structure used to implement and interface with plugin groups.
	 * @tparam Pb Base interface for plugins of this group. */
	template<typename I>
	class plugin_group
	{
	public:
		typedef I interface_type;

	public:
		/* TODO: Implement a static function to get plugin instances of this group for all modules. */
		/* TODO: Implement a static function to invoke an interface member function for every instance. */
	};

	/** @brief Handle used to reference a native dynamic library which contains one or multiple plugins. */
	class module
	{
		template<basic_static_string, template_instance<plugin_group>>
		friend class plugin;

	public:
		typedef detail::module_handle native_handle_type;

	private:
		using path_char = detail::module_path_char;
		using data_t = detail::module_data;

	public:
		/** Returns module handle referencing the main (parent executable) module. */
		[[nodiscard]] static SEK_CORE_PUBLIC module main();

	private:
		/* TODO: Register a type-erased plugin instance for the current module. */
		static SEK_CORE_PUBLIC void register_plugin(std::string_view group, detail::plugin_data data) noexcept;
		static SEK_CORE_PUBLIC void unregister_plugin(std::string_view group, detail::plugin_data data) noexcept;

		template<typename T>
		inline static T return_if(expected<T, std::error_code> &&exp)
		{
			if (!exp.has_value()) [[unlikely]]
				throw std::system_error(exp.error());

			if constexpr (!std::is_void_v<T>) return std::move(exp.value());
		}

		constexpr explicit module(data_t *data) noexcept : m_data(data) {}

	public:
		module(const module &) = delete;
		module &operator=(const module &) = delete;

		/** Initializes a closed module handle. */
		constexpr module() noexcept = default;
		/** Closes the module handle if it is open. */
		SEK_CORE_PUBLIC ~module();

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
		 * @note If the module path is `nullptr`, initializes a main (parent executable) module handle. */
		explicit module(const path_char *path) { open(path); }
		/** @copydoc native_file */
		explicit module(const std::filesystem::path &path) : module(path.c_str()) {}

		/** @brief Loads a module.
		 * @param path Path to the module library.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note A module library is guaranteed to be loaded only once. */
		void open(const std::filesystem::path &path) { return open(path.c_str()); }
		/** @copydoc open
		 * @note If the module path is `nullptr`, opens a handle to the main (parent executable) module. */
		SEK_CORE_PUBLIC void open(const path_char *path);
		/** @copybrief open
		 * @param path Path to the module library.
		 * @return `void` or an error code.
		 * @note A module library is guaranteed to be loaded only once. */
		expected<void, std::error_code> open(std::nothrow_t, const std::filesystem::path &path)
		{
			return open(std::nothrow, path.c_str());
		}
		/** @copydoc open
		 * @note If the module path is `nullptr`, opens a handle to the main (parent executable) module. */
		SEK_CORE_PUBLIC expected<void, std::error_code> open(std::nothrow_t, const path_char *path);

		/** @brief Closes an open module handle. If the internal reference counter reaches 0, unloads the module.
		 * @throw std::system_error On implementation-defined system errors.
		 * @note If the module handle references the main (parent executable) module, does nothing. */
		SEK_CORE_PUBLIC void close();
		/** @copybrief close
		 * @return `void` or an error code.
		 * @note If the module references the main (parent executable) module, does nothing. */
		SEK_CORE_PUBLIC expected<void, std::error_code> close(std::nothrow_t);

		/** Checks if the module handle is open. */
		[[nodiscard]] constexpr bool is_open() const noexcept { return m_data != nullptr; }
		/** @copydoc is_open */
		[[nodiscard]] constexpr operator bool() const noexcept { return is_open(); }

		/* TODO: Implement a static function to get plugin instances for a specific group. */

		/** Returns the underlying native library handle. */
		[[nodiscard]] SEK_CORE_PUBLIC native_handle_type native_handle() const noexcept;
		/** Returns path to the underlying native library. */
		[[nodiscard]] SEK_CORE_PUBLIC const std::filesystem::path &path() const noexcept;

		[[nodiscard]] constexpr auto operator<=>(const module &other) const noexcept { return m_data <=> other.m_data; }
		[[nodiscard]] constexpr bool operator==(const module &other) const noexcept { return m_data == other.m_data; }

		constexpr void swap(module &other) noexcept { std::swap(m_data, other.m_data); }
		friend constexpr void swap(module &a, module &b) noexcept { a.swap(b); }

	protected:
		data_t *m_data = nullptr;
	};

	template<basic_static_string N, template_instance<plugin_group> G>
	plugin<N, G>::plugin() : base_t()
	{
		module::register_plugin(type_name<G>(), detail::plugin_data{this});
	}
	template<basic_static_string N, template_instance<plugin_group> G>
	plugin<N, G>::~plugin()
	{
		module::unregister_plugin(type_name<G>(), detail::plugin_data{this});
	}

	/** @brief Plugin interface used for the core plugin group. */
	class SEK_CORE_PUBLIC core_plugin_interface
	{
	public:
		core_plugin_interface() noexcept = default;
		virtual ~core_plugin_interface() = 0;

		/** Function invoked when a plugin is enabled. */
		virtual void enable() = 0;
		/** Function invoked when a plugin is disabled. */
		virtual void disable() = 0;
	};

	extern template class SEK_API_IMPORT plugin_group<core_plugin_interface>;

	/** @brief Core plugin group type (alias for `plugin_group<core_plugin_interface>`). */
	using core_plugin_group = plugin_group<core_plugin_interface>;
	/** @brief Core plugin base type (alias for `plugin<core_plugin_group, Name>`).
	 * @tparam Name Display name of the plugin. */
	template<basic_static_string Name>
	using core_plugin = plugin<Name, core_plugin_group>;

	template<>
	[[nodiscard]] constexpr std::string_view type_name<core_plugin_group>() noexcept
	{
		return "sek::core_plugin_group";
	}
}	 // namespace sek

// clang-format off
/** @brief Macro used to instantiate & register a plugin of type `T` during static initialization of a module.
 * Any additional arguments are passed to the constructor ot `T`.
 *
 * @example
 * @code{cpp}
 * // my_core_plugin.hpp
 * struct my_core_plugin : sek::core_plugin<"My Core Plugin">
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
#define SEK_PLUGIN_INSTANCE(T, ...) template<> T sek::detail::plugin_data::instance<T> = T(__VA_ARGS__);
// clang-format on
