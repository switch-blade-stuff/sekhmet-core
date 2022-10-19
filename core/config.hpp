/*
 * Created by switchblade on 07/07/22
 */

#pragma once

#include <string>

#include "access_guard.hpp"
#include "dense_set.hpp"
#include "detail/basic_pool.hpp"
#include "serialization/json.hpp"
#include "service.hpp"
#include "system/native_file.hpp"
#include "type_info.hpp"
#include "uri.hpp"
#include <shared_mutex>

namespace sek
{
	class config_registry;

	/** @brief Runtime exception thrown by the config registry. */
	class SEK_CORE_PUBLIC config_error : public std::runtime_error
	{
	public:
		config_error() : std::runtime_error("Unknown config registry error") {}
		explicit config_error(std::string &&msg) : std::runtime_error(std::move(msg)) {}
		explicit config_error(const std::string &msg) : std::runtime_error(msg) {}
		explicit config_error(const char *msg) : std::runtime_error(msg) {}
		~config_error() override;
	};

	/** @brief Path-like structure used to uniquely identify a config registry entry.
	 *
	 * Config paths consist of entry names separated by forward slashes `/`. The first entry is the category entry.
	 * Paths are case-sensitive and are always absolute (since there is no "current" config path). Any sequential
	 * separators within the path (ex. `///`) are concatenated, and the leading separator is optional. */
	class cfg_path
	{
	private:
		struct slice_t
		{
			std::size_t first;
			std::size_t last;
		};

	public:
		typedef char value_type;
		typedef std::size_t size_type;
		typedef std::basic_string<value_type> string_type;
		typedef std::basic_string_view<value_type> string_view_type;

	public:
		/** Initializes an empty config path. */
		constexpr cfg_path() noexcept = default;

		/** Initializes a config path from a string. */
		template<typename S>
		cfg_path(const S &str) : m_value(str)
		{
			parse();
		}
		/** @copydoc cfg_path */
		cfg_path(string_type &&str) : m_value(std::move(str)) { parse(); }
		/** Initializes a config path from a pair of iterators. */
		template<typename I, std::sentinel_for<I> S>
		cfg_path(I first, S last) : m_value(first, last)
		{
			parse();
		}

		/** Initializes a config path from a C-style string. */
		cfg_path(const value_type *str) : cfg_path(string_view_type{str}) {}
		/** Initializes a config path from a character array. */
		cfg_path(const value_type *str, size_type n) : cfg_path(string_view_type{str, n}) {}

		/** Returns the amount of elements (entry names) within the path. */
		[[nodiscard]] constexpr size_type elements() const noexcept { return m_slices.size(); }
		/** Checks if the config path is empty. */
		[[nodiscard]] constexpr bool empty() const noexcept { return elements() == 0; }
		/** Checks if the config path is a category path. */
		[[nodiscard]] constexpr bool is_category() const noexcept { return elements() == 1; }

		/** Returns reference to the underlying path string. */
		[[nodiscard]] constexpr string_type &string() noexcept { return m_value; }
		/** @copydoc string */
		[[nodiscard]] constexpr const string_type &string() const noexcept { return m_value; }

		/** Converts the path to a string view. */
		[[nodiscard]] constexpr operator string_view_type() noexcept { return {m_value}; }

		/** Returns the category component of the path. */
		[[nodiscard]] cfg_path category() const
		{
			auto ptr = &m_slices.back();
			return to_component(ptr, ptr + 1);
		}
		/** Returns the parent entry path. */
		[[nodiscard]] cfg_path parent_path() const
		{
			return to_component(m_slices.data(), m_slices.data() + m_slices.size() - 1);
		}
		/** Returns the entry path without the category component. */
		[[nodiscard]] cfg_path entry_path() const
		{
			return to_component(m_slices.data() + 1, m_slices.data() + m_slices.size());
		}
		/** Returns the last entry name of the path (ex. for path 'graphics/quality' will return `quality`). */
		[[nodiscard]] cfg_path entry_name() const
		{
			auto ptr = &m_slices.back();
			return to_component(ptr, ptr + 1);
		}

		/** Appends another path to this path.
		 * @return Reference to this path. */
		cfg_path &append(const cfg_path &path)
		{
			m_value.append(path.m_value);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &operator/=(const cfg_path &path) { return append(path); }
		/** Returns a path produced from concatenating two paths.
		 * @return Concatenated path. */
		[[nodiscard]] cfg_path operator/(const cfg_path &path) const
		{
			auto tmp = *this;
			tmp /= path;
			return tmp;
		}

		/** Appends a string to the path.
		 * @return Reference to this path. */
		template<typename S>
		cfg_path &append(const S &str)
		{
			m_value.append(str);
			parse();
			return *this;
		}
		/** @copydoc append */
		cfg_path &append(const value_type *str) { return append(string_view_type{str}); }
		/** @copydoc append */
		cfg_path &append(const value_type *str, size_type n) { return append(string_view_type{str, n}); }
		/** @copydoc append */
		template<typename S>
		cfg_path &operator/=(const S &str)
		{
			return append(string_view_type{str});
		}
		/** @copydoc append */
		cfg_path &operator/=(const char *str) { return append(string_view_type{str}); }

		/** Returns a path produced from concatenating a path with a string.
		 * @return Concatenated path. */
		template<typename S>
		[[nodiscard]] cfg_path operator/(const S &str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}
		/** @copydoc operator/ */
		[[nodiscard]] cfg_path operator/(const char *str) const
		{
			auto tmp = *this;
			tmp /= str;
			return tmp;
		}

		[[nodiscard]] constexpr auto operator<=>(const cfg_path &other) noexcept { return m_value <=> other.m_value; }
		[[nodiscard]] constexpr bool operator==(const cfg_path &other) noexcept { return m_value == other.m_value; }

		[[nodiscard]] friend constexpr auto operator<=>(const string_type &a, const cfg_path &b) noexcept
		{
			return a <=> b.m_value;
		}
		[[nodiscard]] friend constexpr bool operator==(const string_type &a, const cfg_path &b) noexcept
		{
			return a == b.m_value;
		}
		[[nodiscard]] friend constexpr auto operator<=>(const cfg_path &a, const string_type &b) noexcept
		{
			return a.m_value <=> b;
		}
		[[nodiscard]] friend constexpr bool operator==(const cfg_path &a, const string_type &b) noexcept
		{
			return a.m_value == b;
		}

		[[nodiscard]] friend constexpr auto operator<=>(string_view_type a, const cfg_path &b) noexcept
		{
			return a <=> b.m_value;
		}
		[[nodiscard]] friend constexpr bool operator==(string_view_type a, const cfg_path &b) noexcept
		{
			return a == b.m_value;
		}
		[[nodiscard]] friend constexpr auto operator<=>(const cfg_path &a, string_view_type b) noexcept
		{
			return a.m_value <=> b;
		}
		[[nodiscard]] friend constexpr bool operator==(const cfg_path &a, string_view_type b) noexcept
		{
			return a.m_value == b;
		}

		[[nodiscard]] friend constexpr auto operator<=>(const value_type *a, const cfg_path &b) noexcept
		{
			return string_view_type{a} <=> b.m_value;
		}
		[[nodiscard]] friend constexpr bool operator==(const value_type *a, const cfg_path &b) noexcept
		{
			return string_view_type{a} == b.m_value;
		}
		[[nodiscard]] friend constexpr auto operator<=>(const cfg_path &a, const value_type *b) noexcept
		{
			return a.m_value <=> string_view_type{b};
		}
		[[nodiscard]] friend constexpr bool operator==(const cfg_path &a, const value_type *b) noexcept
		{
			return a.m_value == string_view_type{b};
		}

		constexpr void swap(cfg_path &other) noexcept
		{
			using std::swap;
			swap(m_value, other.m_value);
			swap(m_slices, other.m_slices);
		}
		friend constexpr void swap(cfg_path &a, cfg_path &b) noexcept { a.swap(b); }

	private:
		[[nodiscard]] SEK_CORE_PUBLIC cfg_path to_component(const slice_t *first, const slice_t *last) const;

		SEK_CORE_PUBLIC void parse();

		/** String containing the full normalized path. */
		string_type m_value;
		/** Individual elements of the path. */
		std::vector<slice_t> m_slices;
	};

	namespace attributes
	{
		class config_type;
	}

	/** @brief Service used to manage configuration entries.
	 *
	 * Engine configuration is stored as entries within the config registry.
	 * Every entry belongs to a category, and is created at plugin initialization time.
	 * Categories are de-serialized from individual Json files or loaded directly from Json data trees.
	 * When a new entry is added, it is de-serialized from the cached category tree. */
	class config_registry : public service<shared_guard<config_registry>>
	{
		friend attributes::config_type;
		friend shared_guard<config_registry>;

		struct entry_node;

		struct entry_hash
		{
			typedef std::true_type is_transparent;

			hash_t operator()(std::string_view key) const noexcept { return fnv1a(key.data(), key.size()); }
			hash_t operator()(const std::string &key) const noexcept { return operator()(std::string_view{key}); }
			hash_t operator()(const entry_node *node) const noexcept { return operator()(node->path.string()); }
		};
		struct entry_cmp
		{
			typedef std::true_type is_transparent;

			hash_t operator()(const entry_node *a, std::string_view b) const noexcept { return a->path == b; }
			hash_t operator()(std::string_view a, const entry_node *b) const noexcept { return a == b->path; }
			hash_t operator()(const entry_node *a, const std::string &b) const noexcept { return a->path == b; }
			hash_t operator()(const std::string &a, const entry_node *b) const noexcept { return a == b->path; }
			bool operator()(const entry_node *a, const entry_node *b) const noexcept { return a == b; }
		};

		using entry_set = dense_set<entry_node *, entry_hash, entry_cmp>;

		// clang-format off
		using output_archive = json::basic_output_archive<json::inline_arrays |
																		 json::extended_fp |
																		 json::pretty_print>;
		using input_archive = json::basic_input_archive<json::allow_comments |
																	   json::extended_fp>;
		// clang-format on
		using output_frame = typename output_archive::archive_frame;
		using input_frame = typename input_archive::archive_frame;

		struct entry_node
		{
			constexpr explicit entry_node(cfg_path &&path) noexcept : path(std::move(path)) {}
			~entry_node()
			{
				for (auto *child : nodes) std::destroy_at(child);
				delete data_cache;
			}

			struct nodes_proxy;
			struct any_proxy;

			SEK_CORE_PUBLIC void serialize(output_frame &, const config_registry &) const;
			SEK_CORE_PUBLIC void deserialize(input_frame &, const config_registry &);
			void deserialize(input_frame &, std::vector<entry_node *> &, const config_registry &);

			cfg_path path;	 /* Full path of the entry. */
			entry_set nodes; /* Immediate children of the entry (if any). */

			/* Optional value of the entry (present if the entry is initialized). */
			any value = {};

			/* Optional cached Json tree of the entry. */
			json_tree *data_cache = nullptr;
		};

		template<bool IsConst>
		class entry_ref;
		template<bool IsConst>
		class entry_ptr;
		template<bool IsConst>
		class entry_iterator;

		template<bool IsConst>
		class entry_ref
		{
			template<bool>
			friend class entry_ref;
			friend class config_registry;

			constexpr explicit entry_ref(entry_node *node) noexcept : m_node(node) {}

		public:
			typedef entry_ref value_type;
			typedef entry_iterator<IsConst> iterator;
			typedef entry_iterator<true> const_iterator;

		private:
			typedef std::conditional_t<IsConst, const any, any> any_type;

		public:
			entry_ref() = delete;

			/** Returns pointer to self. */
			[[nodiscard]] constexpr entry_ref *operator&() noexcept { return this; }
			/** @copydoc operator& */
			[[nodiscard]] constexpr const entry_ref *operator&() const noexcept { return this; }

			/** Returns iterator to the first child of the entry. */
			[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_node->nodes.begin()}; }
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator begin() const noexcept
			{
				return const_iterator{m_node->nodes.begin()};
			}
			/** @copydoc begin */
			[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
			/** Returns iterator one past the last child of the entry. */
			[[nodiscard]] constexpr iterator end() noexcept { return iterator{m_node->nodes.end()}; }
			/** @copydoc end */
			[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{m_node->nodes.end()}; }
			/** @copydoc end */
			[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

			/** Returns reference to the first child of the entry. */
			[[nodiscard]] constexpr entry_ref front() noexcept { return *begin(); }
			/** @copydoc front */
			[[nodiscard]] constexpr entry_ref<true> front() const noexcept { return *begin(); }
			/** Returns reference to the last child of the entry. */
			[[nodiscard]] constexpr entry_ref back() noexcept { return *(--end()); }
			/** @copydoc back */
			[[nodiscard]] constexpr entry_ref<true> back() const noexcept { return *(--end()); }

			/** Returns reference to the config path of the entry. */
			[[nodiscard]] constexpr const cfg_path &path() const noexcept { return m_node->path; }
			/** Returns reference to the value of the entry. */
			[[nodiscard]] constexpr any_type &value() const noexcept { return m_node->value; }

			[[nodiscard]] constexpr auto operator<=>(const entry_ref &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const entry_ref &) const noexcept = default;

			constexpr void swap(entry_ref &other) noexcept { std::swap(m_node, other.m_node); }
			friend constexpr void swap(entry_ref &a, entry_ref &b) noexcept { a.swap(b); }

		private:
			entry_node *m_node = nullptr;
		};
		template<bool IsConst>
		class entry_ptr
		{
			template<bool>
			friend class entry_iterator;
			friend class config_registry;

			constexpr explicit entry_ptr(entry_node *node) noexcept : m_ref(node) {}

		public:
			typedef entry_ref<IsConst> value_type;
			typedef entry_ref<IsConst> reference;
			typedef const value_type *pointer;

		public:
			/** Initializes a null entry pointer. */
			constexpr entry_ptr() noexcept = default;

			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr entry_ptr(const entry_ptr<OtherConst> &other) noexcept : m_ref(other.m_ref)
			{
			}

			/** Checks if the entry pointer is null. */
			[[nodiscard]] constexpr operator bool() const noexcept { return m_ref.m_node != nullptr; }

			/** Returns pointer to the underlying entry. */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return &m_ref; }
			/** Returns reference to the underlying entry. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return m_ref; }

			[[nodiscard]] constexpr auto operator<=>(const entry_ptr &) const noexcept = default;
			[[nodiscard]] constexpr bool operator==(const entry_ptr &) const noexcept = default;

			constexpr void swap(entry_ptr &other) noexcept { m_ref.swap(other.m_ref); }
			friend constexpr void swap(entry_ptr &a, entry_ptr &b) noexcept { a.swap(b); }

		private:
			reference m_ref = reference{nullptr};
		};
		template<bool IsConst>
		class entry_iterator
		{
			friend class config_registry;

			using iter_type = std::conditional_t<IsConst, typename entry_set::const_iterator, typename entry_set::iterator>;

			constexpr explicit entry_iterator(iter_type iter) noexcept : m_iter(iter) {}

		public:
			typedef entry_ref<IsConst> value_type;
			typedef entry_ptr<IsConst> pointer;
			typedef entry_ref<IsConst> reference;
			typedef std::size_t size_type;
			typedef std::ptrdiff_t difference_type;
			typedef std::bidirectional_iterator_tag iterator_category;

		public:
			constexpr entry_iterator() noexcept = default;

			template<bool OtherConst, typename = std::enable_if_t<IsConst && !OtherConst>>
			constexpr entry_iterator(const entry_iterator<OtherConst> &other) noexcept : m_iter(other.m_iter)
			{
			}

			constexpr entry_iterator operator++(int) noexcept
			{
				auto temp = *this;
				++(*this);
				return temp;
			}
			constexpr entry_iterator &operator++() noexcept
			{
				++m_iter;
				return *this;
			}
			constexpr entry_iterator operator--(int) noexcept
			{
				auto temp = *this;
				--(*this);
				return temp;
			}
			constexpr entry_iterator &operator--() noexcept
			{
				--m_iter;
				return *this;
			}

			/** Returns pointer to the underlying entry. */
			[[nodiscard]] constexpr pointer get() const noexcept { return pointer{*m_iter}; }
			/** @copydoc get */
			[[nodiscard]] constexpr pointer operator->() const noexcept { return get(); }
			/** Returns reference to the underlying entry. */
			[[nodiscard]] constexpr reference operator*() const noexcept { return *get(); }

			[[nodiscard]] constexpr auto operator<=>(const entry_iterator &other) const noexcept
			{
				return get() <=> other.get();
			}
			[[nodiscard]] constexpr bool operator==(const entry_iterator &other) const noexcept
			{
				return get() == other.get();
			}

			constexpr void swap(entry_iterator &other) noexcept { m_iter.swap(other.m_iter); }
			friend constexpr void swap(entry_iterator &a, entry_iterator &b) noexcept { a.swap(b); }

		private:
			iter_type m_iter;
		};

	public:
		typedef entry_iterator<false> iterator;
		typedef entry_iterator<true> const_iterator;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

	public:
		constexpr config_registry() noexcept = default;
		SEK_CORE_PUBLIC ~config_registry();

		/** Returns entry iterator to the first category of the registry. */
		[[nodiscard]] constexpr iterator begin() noexcept { return iterator{m_categories.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator begin() const noexcept { return const_iterator{m_categories.begin()}; }
		/** @copydoc begin */
		[[nodiscard]] constexpr const_iterator cbegin() const noexcept { return begin(); }
		/** Returns entry iterator one past the last category of the registry. */
		[[nodiscard]] constexpr iterator end() noexcept { return iterator{m_categories.end()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator end() const noexcept { return const_iterator{m_categories.end()}; }
		/** @copydoc end */
		[[nodiscard]] constexpr const_iterator cend() const noexcept { return end(); }

		/** Returns reference to the first category of the registry. */
		[[nodiscard]] constexpr auto front() noexcept { return *begin(); }
		/** @copydoc front */
		[[nodiscard]] constexpr auto front() const noexcept { return *begin(); }
		/** Returns reference to the last category of the registry. */
		[[nodiscard]] constexpr auto back() noexcept { return *(--end()); }
		/** @copydoc back */
		[[nodiscard]] constexpr auto back() const noexcept { return *(--end()); }

		/** Erases all entries of the registry. */
		SEK_CORE_PUBLIC void clear();

		/** Returns entry pointer to the entry with the specified path. */
		[[nodiscard]] SEK_CORE_PUBLIC entry_ptr<false> find(const cfg_path &entry);
		/** @copydoc find */
		[[nodiscard]] SEK_CORE_PUBLIC entry_ptr<true> find(const cfg_path &entry) const;

		/** Creates a config entry of type `T`. If needed, creates empty entries for parents of the branch.
		 * @param entry Full path of the entry.
		 * @param value Value of the entry.
		 * @return Reference to the value of the entry.
		 * @note If a new entry was inserted and there is a Json data cache up the tree, the entry will be deserialized.
		 * @throw config_error If any entry within the resulting branch fails to initialize. */
		template<typename T>
		inline T &try_insert(cfg_path entry, T value = {})
		{
			if (auto existing = find(entry); !existing) [[likely]]
				return insert_impl(std::move(entry), std::move(value));
			else
				return existing->value().template cast<T &>();
		}
		/** Creates or replaces a config entry of type `T`. If needed, creates empty entries for parents of the branch.
		 * @param entry Full path of the entry.
		 * @param value Value of the entry.
		 * @return Reference to the value of the entry.
		 * @note If such entry already exists, value of the entry will be replaced.
		 * @note If there is a Json data cache up the tree, the entry will be deserialized.
		 * @throw config_error If any entry within the resulting branch fails to initialize. */
		template<typename T>
		inline T &insert(cfg_path entry, T value = {})
		{
			if (auto existing = find(entry); !existing) [[likely]]
				return insert_impl(std::move(entry), std::move(value));
			else
				return assign_impl(existing, std::move(value));
		}

		/** Erases the specified config entry and all it's children.
		 * @return `true` If the entry was erased, `false` otherwise. */
		SEK_CORE_PUBLIC bool erase(entry_ptr<true> which);
		/** @copydoc erase */
		inline bool erase(const_iterator which) { return erase(which.operator->()); }
		/** @copydoc erase */
		SEK_CORE_PUBLIC bool erase(const cfg_path &entry);

		/** Loads an entry and all it's children from a Json data tree.
		 * @param entry Full path of the entry.
		 * @param tree Json data tree containing source data.
		 * @param cache If set to true, the data tree will be cached and re-used for de-serialization of new entries.
		 * @return Entry pointer to the loaded entry.
		 * @throw config_error If any entry within the resulting branch fails to initialize. */
		SEK_CORE_PUBLIC entry_ptr<false> load(cfg_path entry, json_tree &&tree, bool cache = true);
		/** Loads an entry and all it's children from a local Json file.
		 * @param entry Full path of the entry.
		 * @param path Path to a Json file containing source data.
		 * @param cache If set to true, the data will be cached and re-used for de-serialization of new entries.
		 * @return Entry pointer to the loaded entry.
		 * @throw config_error If the file fails to open or any entry within the resulting branch fails to initialize. */
		SEK_CORE_PUBLIC entry_ptr<false> load(cfg_path entry, const std::filesystem::path &path, bool cache = true);
		/** Loads an entry and all it's children from a Json file pointed to by a URI.
		 * @param entry Full path of the entry.
		 * @param location URI pointing to a Json file containing source data.
		 * @param cache If set to true, the data will be cached and re-used for de-serialization of new entries.
		 * @return Entry pointer to the loaded entry.
		 * @throw config_error If the file fails to open or any entry within the resulting branch fails to initialize. */
		SEK_CORE_PUBLIC entry_ptr<false> load(cfg_path entry, const uri &location, bool cache = true);

		/** Saves an entry and all it's children to a Json data tree.
		 * @param which Pointer to the entry to be saved.
		 * @param tree Json data tree to store the serialized data in.
		 * @return `true` on success, `false` on failure (entry does not exist). */
		SEK_CORE_PUBLIC bool save(entry_ptr<true> which, json_tree &tree) const;
		/** @brief Saves an entry and all it's children to a Json file.
		 * @param which Pointer to the entry to be saved.
		 * @param path Path to a the file to write Json data to.
		 * @return `true` on success, `false` on failure (entry does not exist).
		 * @throw config_error If the file fails to open. */
		SEK_CORE_PUBLIC bool save(entry_ptr<true> which, const std::filesystem::path &path) const;
		/** @copybrief save
		 * @param which Pointer to the entry to be saved.
		 * @param location URI pointing to the file write Json data to.
		 * @return `true` on success, `false` on failure (entry does not exist).
		 * @throw config_error If the file fails to open. */
		SEK_CORE_PUBLIC bool save(entry_ptr<true> which, const uri &location) const;

		/** Saves an entry and all it's children to a Json data tree.
		 * @param entry Full path of the entry.
		 * @param tree Json data tree to store the serialized data in.
		 * @return `true` on success, `false` on failure (entry does not exist). */
		inline bool save(const cfg_path &entry, json_tree &tree) const { return save(find(entry), tree); }
		/** @brief Saves an entry and all it's children to a Json file.
		 * @param entry Full path of the entry.
		 * @param path Path to a the file to write Json data to.
		 * @return `true` on success, `false` on failure (entry does not exist).
		 * @throw config_error If the file fails to open. */
		inline bool save(const cfg_path &entry, const std::filesystem::path &path) const
		{
			return save(find(entry), path);
		}
		/** @copybrief save
		 * @param entry Full path of the entry.
		 * @param location URI pointing to the file write Json data to.
		 * @return `true` on success, `false` on failure (entry does not exist).
		 * @throw config_error If the file fails to open. */
		inline bool save(const cfg_path &entry, const uri &location) const { return save(find(entry), location); }

		/* TODO: Rework config saving & loading to use std::error_code instead. */

	private:
		bool save_impl(entry_ptr<true> which, output_archive &archive) const;

		SEK_CORE_PUBLIC entry_node *assign_impl(entry_node *node, any &&value);
		SEK_CORE_PUBLIC entry_node *insert_impl(cfg_path &&entry, any &&value);
		entry_node *insert_impl(cfg_path &&entry);

		entry_node *init_branch(entry_node *node, json_tree *cache);

		template<typename T>
		inline T &insert_impl(cfg_path &&entry, T &&value)
		{
			return insert_impl(std::forward<cfg_path>(entry), make_any<T>(value))->value.template cast<T &>();
		}
		template<typename T>
		inline T &assign_impl(entry_ptr<false> ptr, T &&value)
		{
			return assign_impl(ptr.m_ref.m_node, make_any<T>(value))->value.template cast<T &>();
		}

		void erase_impl(typename entry_set::const_iterator where);
		void clear_impl();

		sek::detail::basic_pool<entry_node> m_node_pool; /* Pool used to allocate entry nodes. */

		entry_set m_categories; /* Categories of the registry. */
		entry_set m_entries;	/* Entry nodes of the registry. */
	};

	namespace attributes
	{
		/** @brief Attribute used to designate a type as a config entry and optionally auto-initialize the entry. */
		class config_type
		{
			friend class config_registry;

			using output_frame = typename config_registry::output_frame;
			using input_frame = typename config_registry::input_frame;

		public:
			template<typename T>
			constexpr explicit config_type(type_selector_t<T>) noexcept
			{
				serialize = [](const any &a, output_frame &frame, const config_registry &reg)
				{
					auto &value = a.template cast<const T &>();
					if constexpr (requires { value.serialize(frame, reg); })
						value.serialize(frame, reg);
					else
					{
						using sek::serialize;
						serialize(value, frame, reg);
					}
				};
				deserialize = [](any &a, input_frame &frame, const config_registry &reg)
				{
					auto &value = a.template cast<T &>();
					if constexpr (requires { value.deserialize(frame, reg); })
						value.deserialize(frame, reg);
					else
					{
						using sek::deserialize;
						deserialize(value, frame, reg);
					}
				};
			}
			template<typename T>
			config_type(type_selector_t<T> s, cfg_path path) : config_type(s)
			{
				config_registry::instance()->access()->template insert<T>(std::move(path));
			}

		private:
			void (*serialize)(const any &, output_frame &, const config_registry &);
			void (*deserialize)(any &, input_frame &, const config_registry &);
		};

		/** Helper function used to create an instance of `config_type` attribute for type `T`. */
		template<typename T>
		[[nodiscard]] constexpr config_type make_config_type() noexcept
		{
			return config_type{type_selector<T>};
		}
		/** @copydoc make_config_type
		 * @param path Config path to create an entry for. */
		template<typename T>
		[[nodiscard]] inline config_type make_config_type(cfg_path path) noexcept
		{
			return config_type{type_selector<T>, path};
		}
	}	 // namespace attributes
}	 // namespace sek