// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_UTILITIES_HPP_
#define ROCKET_UTILITIES_HPP_

#include <type_traits> // std::common_type<>
#include <iterator> // std::iterator_traits<>
#include <utility> // std::swap(), std::move(), std::forward()
#include <cstddef> // std::size_t, std::ptrdiff_t

namespace rocket {

namespace noadl = ::rocket;

using ::std::common_type;
using ::std::iterator_traits;
using ::std::size_t;
using ::std::ptrdiff_t;

template<typename withT, typename typeT>
inline typeT exchange(typeT &ref, withT &&with){
	auto old = ::std::move(ref);
	ref = ::std::forward<withT>(with);
	return old;
}
template<typename lhsT, typename rhsT>
inline void adl_swap(lhsT &lhs, rhsT &rhs){
	using ::std::swap;
	swap(lhs, rhs);
}

template<typename lhsT, typename rhsT>
constexpr typename common_type<lhsT &&, rhsT &&>::type min(lhsT &&lhs, rhsT &&rhs){
	return !(rhs < lhs) ? ::std::forward<lhsT>(lhs) : ::std::forward<rhsT>(rhs);
}
template<typename lhsT, typename rhsT>
constexpr typename common_type<lhsT &&, rhsT &&>::type max(lhsT &&lhs, rhsT &&rhs){
	return !(lhs < rhs) ? ::std::forward<lhsT>(lhs) : ::std::forward<rhsT>(rhs);
}

namespace details_utilities {
	template<typename iteratorT>
	constexpr size_t estimate_distance(::std::input_iterator_tag, iteratorT /*first*/, iteratorT /*last*/){
		return 0;
	}
	template<typename iteratorT>
	inline size_t estimate_distance(::std::forward_iterator_tag, iteratorT first, iteratorT last){
		size_t total = 0;
		for(auto it = ::std::move(first); it != last; ++it){
			++total;
		}
		return total;
	}
	template<typename iteratorT>
	constexpr size_t estimate_distance(::std::random_access_iterator_tag, iteratorT first, iteratorT last){
		return last - first;
	}
}

template<typename iteratorT>
constexpr size_t estimate_distance(iteratorT first, iteratorT last){
	return details_utilities::estimate_distance(typename iterator_traits<iteratorT>::iterator_category(),
	                                            ::std::move(first), ::std::move(last));
}

}

#endif
