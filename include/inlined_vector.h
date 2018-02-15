// Customise the behaviour of inlined_vector by defining these before including it:
// - #define INLINED_VECTOR_THROWS to get runtime_error
// - #define INLINED_VECTOR_LOG_ERROR(message) to log errors

#ifndef INLINED_VECTOR_H
#define INLINED_VECTOR_H

#include <algorithm>
#include <array>
#include <cassert>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

// A vector-like container with inlined storage
// Optional: can grow beyond initial InitialCapacity (using a std::vector)
template<typename T, int InitialCapacity, bool CanExpand = false> class inlined_vector {
	static_assert(InitialCapacity > 0, "InitialCapacity should be > 0");

public:
	using array_type = std::array<T, InitialCapacity>;
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
	inlined_vector() = default;

	template<int InitialCapacity_, bool CanExpand_>
	inlined_vector(const inlined_vector<T, InitialCapacity_, CanExpand_>& other)
		: inlined_vector(other.begin(), other.size()) {}

	template<class Container>
	inlined_vector(const Container& els) : inlined_vector(els.begin(), els.size()) {}

	inlined_vector(std::initializer_list<T> els) : inlined_vector(els.begin(), els.size()) {}

	constexpr static inline int max_size() { return InitialCapacity; }

	inline virtual bool can_expand() const { return false; }

	inline void clear() { size_ = 0; }

	inline unsigned int size() const { return size_; }

	inline bool empty() const { return size_ == 0; }

	inline bool full() const { return size_ >= max_size(); }

	inline virtual bool expanded() const { return false; }

	inline void push_back(const T& value) {
		if (size_ >= max_size()) {
			error("inlined_vector::push_back exceeded InitialCapacity");
		}
		else {
			data_internal_[size_++] = value;
		}
	}

	inline void push_back(T&& rvalue) {
		if (size_ >= max_size()) {
			error("inlined_vector::push_back exceeded InitialCapacity");
		}
		else {
			data_internal_[size_++] = std::move(rvalue);
		}
	}

	template<class... Args> inline void emplace_back(Args&&... args) {
		if (size_ >= max_size()) {
			error("inlined_vector::emplace_back exceeded InitialCapacity");
		}
		else {
			data_internal_[size_] = std::move(T(std::forward<Args>(args)...));
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

	inline void pop() {
		if (!empty())
			size_--;
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

	inline T& operator[](unsigned int i) { return element(i); }

	inline const T& operator[](unsigned int i) const { return element(i); }

	inline const T& at(unsigned int i) const {
		if (i >= 0 && i < size_) {
			return element(i);
		}
		else {
			throw std::out_of_range("inlined_vector::at");
		}
	}

	inline T& at(unsigned int i) {
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

		unsigned int i = iterator_index(it);
		if (i == size_) {
			error("inlined_vector::insert invalid iterator");
			return end();
		}
		for (unsigned int j = i; j < size_ - 1; j++) {
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
			unsigned int i = iterator_index(it);
			if (i == size_) {
				error("inlined_vector::insert invalid iterator");
				return end();
			}
			for (unsigned int j = size_; j > i; j--) {
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
	unsigned int size_ = 0;

protected:
	// Helper constructor
	template<typename Iter> inlined_vector(Iter begin_, unsigned int size) {
		if (size > max_size()) {
			error("inlined_vector() too many elements");
			size_ = max_size();
		}
		else {
			size_ = size;
		}
		std::move(begin_, std::next(begin_, size_), data_internal_.begin());
	}

	// Helper constructor for sub-class
	inlined_vector(array_type&& array, unsigned int size)
		: data_internal_(std::move(array)), size_(size) {}

	inline T& element(unsigned int index) { return *std::next(begin(), index); }

	inline const T& element(unsigned int index) const { return *std::next(begin(), index); }

	unsigned int iterator_index(const_iterator it) const {
		auto nit = begin();
		for (unsigned int i = 0; i < size_; i++) {
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

	template<typename T2, int N>
	friend std::ostream& operator<<(std::ostream& out, const inlined_vector<T2, N, false>& vector);
};

template<typename T, int InitialCapacity>
class inlined_vector<T, InitialCapacity, true> : public inlined_vector<T, InitialCapacity, false> {
public:
	using base_t = inlined_vector<T, InitialCapacity, false>;
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

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

	template<int InitialCapacity_, bool CanExpand_>
	inlined_vector(const inlined_vector<T, InitialCapacity_, CanExpand_>& other)
		: inlined_vector(other.begin(), other.end(), other.size()) {}

	template<class Container>
	inlined_vector(const Container& els) : inlined_vector(els.begin(), els.end(), els.size()) {}

	inlined_vector(std::initializer_list<T> els)
		: inlined_vector(els.begin(), els.end(), els.size()) {}

	inlined_vector(const inlined_vector& other)
		: inlined_vector(other.begin(), other.end(), other.size()) {}

	inlined_vector(inlined_vector&& other)
		: base_t(std::move(other.data_internal_), other.size_),
		  data_external_(std::move(other.data_external_)), inlined_(other.inlined_) {
		// TODO: Allow move between different fixed_vectors
		other.inlined_ = true;
		other.size_ = 0;
	}

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

	inline void push_back(const T& value) {
		if (inlined_ && size_ >= max_size()) {
			grow_to_external_storage();
		}

		if (inlined_) {
			data_internal_[size_++] = value;
		}
		else {
			data_external_.push_back(value);
			size_++;
		}
	}

	inline void push_back(T&& rvalue) {
		if (inlined_ && size_ >= max_size()) {
			grow_to_external_storage();
		}

		if (inlined_) {
			data_internal_[size_++] = std::move(rvalue);
		}
		else {
			data_external_.emplace_back(std::move(rvalue));
			size_++;
		}
	}

	template<class... Args> inline void emplace_back(Args&&... args) {
		if (inlined_ && size_ >= max_size()) {
			grow_to_external_storage();
		}

		if (inlined_) {
			// No emplace for array
			data_internal_[size_] = std::move(T(std::forward<Args>(args)...));
			size_++;
		}
		else {
			data_external_.emplace_back(std::forward<Args>(args)...);
			size_++;
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
			unsigned int i = base_t::iterator_index(it);
			if (i == size_) {
				error("inlined_vector::erase invalid iterator");
				return end();
			}
			for (unsigned int j = i; j < size_ - 1; j++) {
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
			unsigned int index_ = base_t::iterator_index(it);
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
	template<typename Iter> inlined_vector(Iter begin_, Iter end_, unsigned int size) {
		size_ = size;
		if (size_ <= max_size()) {
			std::move(begin_, end_, data_internal_.begin());
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
		data_external_.resize(size_);
		std::move(data_internal_.begin(), data_internal_.begin() + size_, data_external_.begin());
		inlined_ = false;
	}

	template<typename T2, int N>
	friend std::ostream& operator<<(std::ostream& out, const inlined_vector<T2, N, true>& vector);
};

template<typename T, int N>
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

template<typename T, int N>
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