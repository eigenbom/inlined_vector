// Customise the behaviour of inlined_vector by defining these before including it:
// - #define INLINED_VECTOR_THROWS to get runtime_error
// - #define INLINED_VECTOR_LOG_ERROR(message) to log errors

#ifndef INLINED_VECTOR_H
#define INLINED_VECTOR_H

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "static_vector.h"

// A vector-like container with inlined storage
// Optional: can grow beyond initial InitialCapacity (using a std::vector)
template<typename T, std::size_t InitialCapacity, bool CanExpand = false> class inlined_vector {
public:
	using array_type = static_vector<T, InitialCapacity>; // std::array<T, InitialCapacity>;
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;
	using size_type = std::size_t;

public:
	inlined_vector() = default;

	template<std::size_t InitialCapacity_, bool CanExpand_>
	inlined_vector(const inlined_vector<T, InitialCapacity_, CanExpand_>& other)
		: inlined_vector(other.begin(), other.size()) {}

    template<std::size_t InitialCapacity_, bool CanExpand_>
    inlined_vector(inlined_vector<T, InitialCapacity_, CanExpand_>&& other)
        : inlined_vector(other.begin(), other.size()) {}

	template<class Container>
	inlined_vector(const Container& els) : inlined_vector(els.begin(), els.size()) {}

	inlined_vector(std::initializer_list<T> els) : inlined_vector(els.begin(), els.size()) {}

	constexpr static inline size_type max_size() { return InitialCapacity; }

	inline virtual bool can_expand() const { return false; }

	inline void clear() { size_ = 0; }

	inline size_type size() const { return size_; }

	inline bool empty() const { return size_ == 0; }

	inline bool full() const { return size_ >= max_size(); }

	inline virtual bool expanded() const { return false; }

	template <typename U>
	inline void push_back(U&& value) {
		if (size_ >= max_size()) {
			error("inlined_vector::push_back exceeded InitialCapacity");
		}
		else {
			data_internal_.push_back(std::forward<U>(value));
			size_++;
		}
	}

	template<class... Args> inline void emplace_back(Args&&... args) {
		if (size_ >= max_size()) {
			error("inlined_vector::emplace_back exceeded InitialCapacity");
		}
		else {
			data_internal_.emplace_back(std::forward<Args>(args)...);
			size_++;
		}
	}

	template<class Container> void extend(const Container& other) {
		for (auto v : other) {
			push_back(std::move(v));
		}
	}

	void extend(std::initializer_list<T> other) {
		for (auto v : other) {
			push_back(std::move(v));
		}
	}

	inline virtual void pop_back() {
		if (!empty()) size_--;
	}

	inline const T& back() const {
		if (!empty()) {
			return *std::prev(end());
		}
		return data_internal_[0];
	}

	inline T& back() { return const_cast<T&>(static_cast<const inlined_vector*>(this)->back()); }

	inline const T& front() const {
		if (!empty()) {
			return *begin();
		}
		return data_internal_[0];
	}

	inline T& front() { return const_cast<T&>(static_cast<const inlined_vector*>(this)->front()); }

	inline T& operator[](size_type i) { return element(i); }

	inline const T& operator[](size_type i) const { return element(i); }

	inline const T& at(size_type i) const {
		if (i >= 0 && i < size_) {
			return element(i);
		}
		else {
			throw std::out_of_range("inlined_vector::at");
		}
	}

	inline T& at(size_type i) {
		return const_cast<T&>(static_cast<const inlined_vector*>(this)->at(i));
	}

	virtual iterator begin() { return data_internal_.begin(); }
	iterator end() { return begin() + size_; }

	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }

	virtual const_iterator begin() const { return data_internal_.begin(); }
	const_iterator end() const { return begin() + size_; }

	reverse_iterator rbegin() { return rend() - size_; }
	virtual reverse_iterator rend() { return data_internal_.rend(); }

	const_reverse_iterator rbegin() const { return rend() - size_; }
	virtual const_reverse_iterator rend() const { return data_internal_.rend(); }

	iterator erase(const_iterator it) {
		validate_iterator(it);

		if (it == end() || empty()) {
			error("inlined_vector::erase it == end or container is empty");
		}

		size_type i = iterator_index(it);
		if (i == size_) {
			error("inlined_vector::insert invalid iterator");
			return end();
		}
		for (size_type j = i; j < size_ - 1; j++) {
			element(j) = std::move(element(j + 1));
		}
		size_--;
		return begin() + i;
	}

	iterator insert(iterator it, const T& value) {
		validate_iterator(it);

		if (full()) {
			error("inlined_vector::insert exceeded InitialCapacity");
			return end();
		}

		if (it == end()) {
			push_back(value);
			return end();
		}
		else {
			// Insert at i and push everything back
			size_type i = iterator_index(it);
			if (i == size_) {
				error("inlined_vector::insert invalid iterator");
				return end();
			}
			for (size_type j = size_; j > i; j--) {
				element(j) = std::move(element(j - 1));
			}
			element(i) = value;
			size_++;
			return std::next(begin(), i);
		}
	}

	inline bool contains(const T& value) const {
		auto begin_ = begin();
		auto end_ = end();
		return std::find(begin_, end_, value) != end_;
	}

protected:
	array_type data_internal_;
	size_type size_ = 0;

protected:
	// Helper constructor
	template<typename Iter> inlined_vector(Iter begin_, std::size_t size) {
		if (size > max_size()) {
			error("inlined_vector() too many elements");
			size_ = max_size();
		}
		else {
			size_ = size;
		}

		auto end_ = std::next(begin_, size_);
		for (auto it = begin_; it != end_; ++it){
			data_internal_.emplace_back(std::move(*it));
		}
		// std::move(begin_, std::next(begin_, size_), data_internal_.begin());
	}

	// Helper constructor for sub-class
	inlined_vector(array_type&& array, std::size_t size)
		: data_internal_(std::move(array)), size_(size) {}

	inline T& element(size_type index) { return *std::next(begin(), index); }

	inline const T& element(size_type index) const { return *std::next(begin(), index); }

	size_type iterator_index(const_iterator it) const {
		auto nit = begin();
		for (size_type i = 0; i < size_; i++) {
			if (nit == it)
				return i;
			nit++;
		}
		return size_;
	}

	inline void validate_iterator(const_iterator it) {
#ifndef NDEBUG
		if (it < begin() || it > end()) {
			error("inlined_vector::validate_iterator invalid iterator");
		}
#endif
	}

	void error(const char* message) const {
#ifdef INLINED_VECTOR_LOG_ERROR
		INLINED_VECTOR_LOG_ERROR(message);
#endif

#ifdef INLINED_VECTOR_THROWS
		throw std::runtime_error(message);
#endif
	}

	template<typename T2, std::size_t N>
	friend std::ostream& operator<<(std::ostream& out, const inlined_vector<T2, N, false>& vector);
};

template<typename T, std::size_t InitialCapacity>
class inlined_vector<T, InitialCapacity, true> : public inlined_vector<T, InitialCapacity, false> {
public:
	using base_t = inlined_vector<T, InitialCapacity, false>;
	using typename base_t::value_type;
	using typename base_t::iterator;
	using typename base_t::const_iterator;
	using typename base_t::reverse_iterator;
	using typename base_t::const_reverse_iterator;
	using typename base_t::size_type;
	using base_t::cbegin;
	using base_t::element;
	using base_t::empty;
	using base_t::end;
	using base_t::error;
	using base_t::max_size;
	using base_t::rbegin;
	using base_t::data_internal_;
	using base_t::size_;

public:
	inlined_vector() = default;

	template<std::size_t InitialCapacity_, bool CanExpand_>
	inlined_vector(const inlined_vector<T, InitialCapacity_, CanExpand_>& other)
		: inlined_vector(other.begin(), other.end(), other.size()) {}

    inlined_vector(inlined_vector&& other)
        : base_t(std::move(other.data_internal_), other.size_),
		  data_external_(std::move(other.data_external_)), 
          inlined_(other.inlined_) {
        other.inlined_ = true;
        other.size_ = 0;
    }

	template<class Container>
	inlined_vector(const Container& els) : inlined_vector(els.begin(), els.end(), els.size()) {}

	inlined_vector(std::initializer_list<T> els)
		: inlined_vector(els.begin(), els.end(), els.size()) {}

	inlined_vector(const inlined_vector& other)
		: inlined_vector(other.begin(), other.end(), other.size()) {}

	inlined_vector& operator=(inlined_vector&& other) {
		inlined_ = other.inlined_;
		size_ = other.size_;
		data_internal_ = std::move(other.data_internal_);
		data_external_ = std::move(other.data_external_);
		other.inlined_ = true;
		other.size_ = 0;
		return *this;
	}

	inlined_vector& operator=(const inlined_vector& other) {
		inlined_ = other.inlined_;
		size_ = other.size_;
		data_internal_ = other.data_internal_;
		data_external_ = other.data_external_;
		return *this;
	}

	template<class Container> void extend(const Container& other) {
		for (auto v : other) {
			push_back(std::move(v));
		}
	}

	void extend(std::initializer_list<T> other) {
		for (auto v : other) {
			push_back(std::move(v));
		}
	}

	inline virtual bool can_expand() const override final { return true; }

	inline void clear() {
		if (!inlined_) {
			inlined_ = false;
			data_external_.clear();
		}
		base_t::clear();
	}

	inline bool expanded() const final override { return !inlined_; }

	template <typename U>
	inline void push_back(U&& value) {
		if (inlined_ && size_ >= max_size()) {
			grow_to_external_storage();
		}

		if (inlined_) {
			base_t::push_back(std::forward<U>(value));
		}
		else {
			data_external_.push_back(std::forward<U>(value));
			size_++;
		}
	}

	template<class... Args> inline void emplace_back(Args&&... args) {
		if (inlined_ && size_ >= max_size()) {
			grow_to_external_storage();
		}

		if (inlined_) {
			base_t::emplace_back(std::forward<Args>(args)...);
		}
		else {
			data_external_.emplace_back(std::forward<Args>(args)...);
			size_++;
		}
	}

	inline void pop_back() override final {
		if (!empty()){
			if (inlined_) data_external_.pop_back();
			size_--;
		}
	}

	iterator begin() override final {
		return inlined_ ? data_internal_.begin() : unwrap(data_external_.begin());
	}
	const_iterator begin() const override final {
		return inlined_ ? data_internal_.begin() : unwrap(data_external_.begin());
	}
	reverse_iterator rend() override final {
		return inlined_ ? data_internal_.rend() : unwrap(data_external_.rend());
	}
	const_reverse_iterator rend() const override final {
		return inlined_ ? data_internal_.rend() : unwrap(data_external_.rend());
	}

	iterator erase(const_iterator it) {
		base_t::validate_iterator(it);

		if (it == end() || empty()) {
			error("inlined_vector::erase it == end or container is empty");
		}

		if (inlined_) {
			size_type i = base_t::iterator_index(it);
			if (i == size_) {
				error("inlined_vector::erase invalid iterator");
				return end();
			}
			for (size_type j = i; j < size_ - 1; j++) {
				element(j) = std::move(element(j + 1));
			}
			size_--;
			return begin() + i;
		}
		else {
			size_--;
			auto vit = std::next(data_external_.cbegin(), std::distance(cbegin(), it));
			return unwrap(data_external_.erase(vit));
		}
	}

	iterator insert(iterator it, const T& value) {
		base_t::validate_iterator(it);

		if (inlined_ && size_ < max_size()) {
			return base_t::insert(it, value);
		}
		else if (inlined_ && size_ >= max_size()) {
			size_type index_ = base_t::iterator_index(it);
			grow_to_external_storage();
			it = std::next(begin(), index_);
		}

		if (it == end()) {
			push_back(value);
			return end();
		}
		else {
			size_++;
			// NB: dataVector may not have a T* iterator
			auto vit = std::next(data_external_.begin(), std::distance(begin(), it));
			return unwrap(data_external_.insert(vit, value));
		}
	}

protected:
	std::vector<T> data_external_;
	bool inlined_ = true;

protected:
	// Helper constructor
	template<typename Iter> inlined_vector(Iter begin_, Iter end_, std::size_t size) {
		size_ = size;
		if (size_ <= max_size()) {
			for (auto it = begin_; it != end_; ++it){
				data_internal_.emplace_back(std::move(*it));
			}
		}
		else {
			data_external_.resize(size_);
			std::move(begin_, end_, data_external_.begin());
			inlined_ = false;
		}
	}

	// Sometimes std::vector<T>::iterator isn't a T* (e.g., in clang/libcxx)
	// so we need to unwrap the iterator
	inline iterator unwrap(typename std::vector<T>::iterator it) const { return &*it; }
	inline const_iterator unwrap(typename std::vector<T>::const_iterator it) const { return &*it; }
	inline reverse_iterator unwrap(typename std::vector<T>::reverse_iterator it) const {
		return reverse_iterator(&*it);
	}
	inline const_reverse_iterator unwrap(typename std::vector<T>::const_reverse_iterator it) const {
		return const_reverse_iterator(&*it);
	}

	void grow_to_external_storage() {
		assert(inlined_);
		data_internal_.emplace_into(data_external_);
		inlined_ = false;
	}

	template<typename T2, std::size_t N>
	friend std::ostream& operator<<(std::ostream& out, const inlined_vector<T2, N, true>& vector);
};

template<typename T, std::size_t N>
inline std::ostream& operator<<(std::ostream& out, const inlined_vector<T, N, false>& vector) {
	out << "inlined_vector ";
	out << "(inlined):  [";
	if (vector.empty())
		out << "]";
	else {
		for (auto it = vector.begin(); it != vector.end(); ++it) {
			if (std::next(it) != vector.end())
				out << *it << ", ";
			else
				out << *it << "]";
		}
	}
	return out;
}

template<typename T, std::size_t N>
inline std::ostream& operator<<(std::ostream& out, const inlined_vector<T, N, true>& vector) {
	out << "inlined_vector ";
	if (vector.inlined_)
		out << "(inlined):  [";
	else
		out << "(external): [";
	if (vector.empty())
		out << "]";
	else {
		for (auto it = vector.begin(); it != vector.end(); ++it) {
			if (std::next(it) != vector.end())
				out << *it << ", ";
			else
				out << *it << "]";
		}
	}
	return out;
}

#endif