/*
 * Created by switchblade on 2022-10-03.
 */

#pragma once

#include "../../dense_map.hpp"
#include "../../dense_set.hpp"
#include "type_info.hpp"

namespace sek
{
	/** @brief Singleton used to store reflection type database. */
	class type_database
	{
		template<typename>
		friend class type_factory;
		friend class type_query;

	public:
		/** Returns guarded pointer to the global type database instance. */
		[[nodiscard]] static SEK_CORE_PUBLIC shared_guard<type_database *> instance();

	private:
		struct type_cmp
		{
			typedef std::true_type is_transparent;

			template<typename T>
			constexpr bool operator()(const T &name, const type_info &type) const noexcept
				requires std::is_convertible_v<T, std::string_view>
			{
				return type.valid() && type.name() == static_cast<std::string_view>(name);
			}
			template<typename T>
			constexpr bool operator()(const type_info &type, const T &name) const noexcept
				requires std::is_convertible_v<T, std::string_view>
			{
				return type.valid() && type.name() == static_cast<std::string_view>(name);
			}
			constexpr bool operator()(const type_info &a, const type_info &b) const noexcept { return a == b; }
		};
		struct type_hash
		{
			typedef std::true_type is_transparent;

			template<typename T>
			constexpr std::size_t operator()(const T &name) const noexcept
				requires std::is_convertible_v<T, std::string_view>
			{
				const auto sv = static_cast<std::string_view>(name);
				return fnv1a(sv.data(), sv.size());
			}
			constexpr std::size_t operator()(const type_info &type) const noexcept { return hash(type); }
		};

		using type_table_t = dense_set<type_info, type_hash, type_cmp>;
		using attr_table_t = dense_map<std::string_view, type_table_t>;

		type_database() = default;

	public:
		/** Adds the type to the internal database & returns a type factory for it.
		 * @return Type factory for type `T`, which can be used to specify additional information about the type.
		 * @throw type_error If the type is already reflected. */
		template<typename T>
		[[nodiscard]] type_factory<T> reflect()
		{
			static_assert(is_exported_type_v<T>, "Reflected type must be exported via `SEK_EXTERN_TYPE_INFO`");
			return type_factory<T>{{this, &m_mtx}, type_info::handle<T>()};
		}

		/** Returns type info for the specified reflected type. */
		[[nodiscard]] SEK_CORE_PUBLIC type_info get(std::string_view type);
		/** Removes a previously reflected type from the internal database. */
		SEK_CORE_PUBLIC void reset(std::string_view type);
		/** @copydoc reset */
		void reset(type_info type) { reset(type.name()); }
		/** @copydoc reset */
		template<typename T>
		void reset()
		{
			reset(type_info::get<T>());
		}

		/** Creates a type query used to filter reflected types. */
		[[nodiscard]] constexpr type_query query() const noexcept;
		/** Returns reference to the internal set of types. */
		[[nodiscard]] constexpr const type_table_t &types() const noexcept { return m_type_table; }

	private:
		SEK_CORE_PUBLIC const detail::type_data *reflect_impl(detail::type_handle);

		mutable std::shared_mutex m_mtx;
		type_table_t m_type_table;
		attr_table_t m_attr_table;
	};

	/** @brief Structure used to obtain a filtered subset of types from the type database. */
	class type_query
	{
		using type_set_t = typename type_database::type_table_t;

	public:
		type_query() = delete;

		/** Creates a type query for the specified type database. */
		constexpr explicit type_query(const type_database &db) : m_db(db) {}

		/** Excludes all types that do not have the specified attribute. */
		[[nodiscard]] SEK_CORE_PUBLIC type_query &with_attribute(type_info type);
		/** @copydoc with_attribute */
		template<typename T>
		[[nodiscard]] type_query &with_attribute()
		{
			return with_attribute(type_info::get<T>());
		}

		/** Excludes all types that do not have a constant of the specified name. */
		[[nodiscard]] SEK_CORE_PUBLIC type_query &with_constant(std::string_view name);
		/** Excludes all types that do not have a constant of the specified name and type. */
		[[nodiscard]] SEK_CORE_PUBLIC type_query &with_constant(std::string_view name, type_info type);
		/** @copydoc with_constant */
		template<typename T>
		[[nodiscard]] type_query &with_constant(std::string_view name)
		{
			return with_constant(name, type_info::get<T>());
		}

		/** Excludes all types that do not inherit from the specified parent. */
		[[nodiscard]] SEK_CORE_PUBLIC type_query &inherits_from(type_info type);
		/** @copydoc with_parent */
		template<typename T>
		[[nodiscard]] type_query &inherits_from()
		{
			return with_parent(type_info::get<T>());
		}

		/** Excludes all types that cannot be converted to the specified type. */
		[[nodiscard]] SEK_CORE_PUBLIC type_query &convertible_to(type_info type);
		/** @copydoc convertible_to */
		template<typename T>
		[[nodiscard]] type_query &convertible_to()
		{
			return convertible_to(type_info::get<T>());
		}

		/** Returns reference to the set of types that was matched by the query. */
		[[nodiscard]] constexpr const type_set_t &types() const noexcept { return m_types; }

	private:
		const type_database &m_db;
		type_set_t m_types;
		bool m_started = false; /* If set to `false`, the type set is safe to overwrite. */
	};

	constexpr type_query type_database::query() const noexcept { return type_query{*this}; }

	// clang-format off
	template<typename T>
	type_factory<T> type_info::reflect() { return type_database::instance()->template reflect<T>(); }
	type_info type_info::get(std::string_view name) { return type_database::instance()->get(name); }
	void type_info::reset(std::string_view name) { type_database::instance()->reset(name); }
	// clang-format on
}	 // namespace sek