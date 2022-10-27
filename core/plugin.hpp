/*
 * Created by switchblade on 12/05/22
 */

#pragma once

#include <filesystem>
#include <functional>
#include <vector>

#include "access_guard.hpp"
#include "expected.hpp"
#include "type_name.hpp"
#include "version.hpp"

namespace sek
{
	class module;

	class plugin_interface;
	template<typename>
	class plugin_group;
	template<basic_static_string, version, template_instance<plugin_group>>
	class plugin;

	namespace detail
	{
		using module_path_char = typename std::filesystem::path::value_type;
		using module_handle = void *;

		struct module_data;
		struct plugin_node
		{
			void link(plugin_node &other) noexcept
			{
				next = &other;
				prev = other.prev;
				other.prev = this;
				if (prev != nullptr) [[likely]]
					prev->next = this;
			}
			void unlink() noexcept
			{
				next->prev = prev;
				prev->next = next;
			}

			plugin_node *next = nullptr;
			plugin_node *prev = nullptr;
		};
		struct group_data
		{
			static SEK_CORE_PUBLIC detail::group_data *instance(std::string_view group) noexcept;

			SEK_CORE_PUBLIC void register_plugin(plugin_interface *ptr) noexcept;
			SEK_CORE_PUBLIC void unregister_plugin(plugin_interface *ptr) noexcept;

			std::recursive_mutex mtx;
			plugin_node plugins = {&plugins, &plugins};
		};

		template<typename P>
		extern SEK_API_IMPORT P plugin_instance;
	}	 // namespace detail

	/** @brief Handle used to reference a native dynamic library which contains one or multiple plugins. */
	class module
	{
		using path_char = detail::module_path_char;
		using data_t = detail::module_data;

	public:
		typedef detail::module_handle native_handle_type;

		/** Returns module handle referencing the main (parent executable) module. */
		[[nodiscard]] static SEK_CORE_PUBLIC module main();
		/** Returns a vector of module handles to all currently opened modules. */
		[[nodiscard]] static SEK_CORE_PUBLIC std::vector<module> all();

	private:
		template<typename T>
		static T return_if(expected<T, std::error_code> &&exp)
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

	/** @brief Structure used to implement common base functionality for all plugin interfaces. */
	class SEK_CORE_PUBLIC plugin_interface : detail::plugin_node
	{
		template<basic_static_string, version, template_instance<plugin_group>>
		friend class plugin;
		template<typename>
		friend class plugin_group;
		friend struct detail::group_data;

	public:
		plugin_interface() noexcept = default;
		virtual ~plugin_interface() = 0;

		/** Checks if the plugin is enabled. */
		[[nodiscard]] constexpr bool is_enabled() const noexcept { return m_enabled; }
		/** Returns the version of core framework the plugin was built for. */
		[[nodiscard]] constexpr version core_ver() const noexcept { return m_core_ver; }
		/** Returns the version of the plugin instance. */
		[[nodiscard]] constexpr version plugin_ver() const noexcept { return m_plugin_ver; }
		/** Returns the display name of the plugin. */
		[[nodiscard]] constexpr std::string_view name() const noexcept { return m_name; }

		/** Function invoked when a plugin is enabled. */
		virtual void enable();
		/** Function invoked when a plugin is disabled.
		 * @note Automatically destroys all plugin-local objets. */
		virtual void disable();

	private:
		bool m_enabled = false;

		version m_core_ver = SEK_CORE_VERSION;
		version m_plugin_ver;

		std::string_view m_name;
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
	class plugin_group : detail::group_data
	{
		template<basic_static_string, version, template_instance<plugin_group>>
		friend class plugin;

		static_assert(std::is_base_of_v<plugin_interface, I>, "Plugin interface must be a specialization of `plugin_interface`");

		using node_t = detail::plugin_node;

	public:
		typedef I interface_type;

		class iterator
		{
			template<typename>
			friend class plugin_group;

		public:
			typedef interface_type value_type;
			typedef interface_type *pointer;
			typedef interface_type &reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		private:
			constexpr explicit iterator(node_t *node) noexcept : m_node(node) {}

		public:
			constexpr iterator() noexcept = default;

			constexpr iterator operator++(int) noexcept
			{
				auto tmp = *this;
				++(*this);
				return tmp;
			}
			constexpr iterator &operator++() noexcept
			{
				m_node = m_node->next;
				return *this;
			}
			constexpr iterator operator--(int) noexcept
			{
				auto tmp = *this;
				--(*this);
				return tmp;
			}
			constexpr iterator &operator--() noexcept
			{
				m_node = m_node->prev;
				return *this;
			}

			/** Returns a pointer to the referenced plugin instance. */
			[[nodiscard]] constexpr pointer get() const noexcept { return static_cast<interface_type *>(m_node); }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns a reference to the referenced plugin instance. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const iterator &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const iterator &) const noexcept = default;

		private:
			node_t *m_node = nullptr;
		};
		typedef std::reverse_iterator<iterator> reverse_iterator;
		typedef typename iterator::size_type size_type;
		typedef typename iterator::difference_type difference_type;

		/** Returns an access guard to the global plugin group instance. */
		[[nodiscard]] static recursive_guard<plugin_group *> instance() noexcept
		{
			static auto *data = detail::group_data::instance(type_name_v<plugin_group>);
			return {static_cast<plugin_group *>(data), &data->mtx};
		}

	public:
		plugin_group() = delete;
		plugin_group(const plugin_group &) = delete;
		plugin_group &operator=(const plugin_group &) = delete;

		/** Returns an iterator to the first plugin of the group. */
		[[nodiscard]] constexpr iterator begin() const noexcept { return iterator{plugins.next}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr iterator cbegin() const noexcept { return begin(); }
		/** Returns an iterator one past the last plugin of the group. */
		[[nodiscard]] constexpr iterator end() const noexcept { return iterator{const_cast<node_t *>(&plugins)}; }
		/** @copydoc end */
		[[nodiscard]] constexpr iterator cend() const noexcept { return end(); }
		/** Returns a reverse iterator to the last plugin of the group. */
		[[nodiscard]] constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
		/** @copydoc rbegin */
		[[nodiscard]] constexpr reverse_iterator crbegin() const noexcept { return rbegin(); }
		/** Returns a reverse iterator one past the first plugin of the group. */
		[[nodiscard]] constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }
		/** @copydoc rend */
		[[nodiscard]] constexpr reverse_iterator crend() const noexcept { return rend(); }

		/** Enables all plugins of this group.
		 * @return Amount of plugins enabled. */
		size_type enable_all()
		{
			size_type result = 0;
			for (plugin_interface &plugin : *this)
				if (!plugin.is_enabled())
				{
					plugin.enable();
					++result;
				}
			return result;
		}
		/** Enables all plugins of this group that match a predicate.
		 * @param pred Predicate used to filter plugins.
		 * @return Amount of plugins enabled. */
		template<typename P>
		size_type enable_if(P &&pred)
		{
			size_type result = 0;
			for (plugin_interface &plugin : *this)
				if (!plugin.is_enabled() && std::invoke(pred, plugin))
				{
					plugin.enable();
					++result;
				}
			return result;
		}

		/** Disables all plugins of this group.
		 * @return Amount of plugins disabled. */
		size_type disable_all()
		{
			size_type result = 0;
			for (plugin_interface &plugin : *this)
				if (plugin.is_enabled())
				{
					plugin.disable();
					++result;
				}
			return result;
		}
		/** Disables all plugins of this group that match a predicate.
		 * @param pred Predicate used to filter plugins.
		 * @return Amount of plugins disabled. */
		template<typename P>
		size_type disable_if(P &&pred)
		{
			size_type result = 0;
			for (plugin_interface &plugin : *this)
				if (plugin.is_enabled() && std::invoke(pred, plugin))
				{
					plugin.disable();
					++result;
				}
			return result;
		}
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

	template<basic_static_string N, version V, template_instance<plugin_group> G>
	plugin<N, V, G>::plugin()
	{
		plugin_interface::m_name = static_string_cast<char>(N);
		plugin_interface::m_plugin_ver = V;
		G::instance()->register_plugin(this);
	}
	template<basic_static_string N, version V, template_instance<plugin_group> G>
	plugin<N, V, G>::~plugin()
	{
		G::instance()->unregister_plugin(this);
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