#ifndef STATIC_VECTOR_H
#define STATIC_VECTOR_H

#include <iterator>
#include <type_traits>
#include <string>

template<class T, int Capacity>
class static_vector
{    
public:
	using value_type = T;
	using iterator = value_type*;
	using const_iterator = const value_type*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:
    static_vector() = default;

    static_vector(const static_vector& st):size_(st.size_){
        for(unsigned int i = 0; i < size_; ++i) {
            new (data_+i) T(st[i]);
        }
    }

    static_vector(static_vector&& st):size_(st.size_){
        for(unsigned int i = 0; i < size_; ++i) {
            new (data_+i) T(std::move(st[i]));
        }
    }

    ~static_vector(){
        for(unsigned int i = 0; i < size_; ++i) {
            destroy(data_+i);
        }
    }

    static_vector& operator=(const static_vector& other){
        for(unsigned int i = 0; i < size_; ++i) {
            destroy(data_+i);
        }
        size_ = other.size_;
        for(unsigned int i = 0; i < size_; ++i) {
            new (data_+i) T(other[i]);
        }
        return *this;
    }

    static_vector& operator=(static_vector&& other){
        for(unsigned int i = 0; i < size_; ++i) {
            destroy(data_+i);
        }
        size_ = other.size_;
        for(unsigned int i = 0; i < size_; ++i) {
            new (data_+i) T(std::move(other[i]));
        }
        return *this;
    }

    void push_back(const T& value) {
        if( size_ >= max_size() ) throw std::bad_alloc{};
        new (data_+size_) T(value);
        ++size_;
    }

    void push_back(T&& value) {
        if( size_ >= max_size() ) throw std::bad_alloc{};
        new (data_+size_) T(std::move(value));
        ++size_;
    }

    template<typename ...Args> void emplace_back(Args&&... args) {
        if( size_ >= max_size() ) throw std::bad_alloc{};
        new (data_+size_) T(std::forward<Args>(args)...);
        ++size_;
    }

    inline int size() const { return size_; }
    constexpr static inline int max_size() { return Capacity; }
 
    T& operator[](unsigned int i){
        return *launder(data_ + i);
    }

    const T& operator[](unsigned int i) const {
        return *launder(data_ + i);
    }

    iterator begin() { return launder(data_); }
	iterator end() { return begin() + size_; }

	const_iterator cbegin() const { return begin(); }
	const_iterator cend() const { return end(); }

	const_iterator begin() const { return launder(data_); }
	const_iterator end() const { return begin() + size_; }

	reverse_iterator rbegin() { return rend() - size_; }
	reverse_iterator rend() { return std::reverse_iterator<iterator>(begin()); }

	const_reverse_iterator rbegin() const { return rend() - size_; }
	const_reverse_iterator rend() const { return std::reverse_iterator<const_iterator>(begin()); }

    template <typename Container> void emplace_into(Container& container){        
        for(unsigned int i = 0; i < size_; ++i) {
            container.emplace_back(std::move(*launder(data_+i)));
            destroy(data_+i);
        }
        size_ = 0;
    }

protected:
    using raw_type = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
    
    raw_type data_[Capacity];
    unsigned int size_ = 0; 

protected:
    T* launder(raw_type* rt){
        return reinterpret_cast<T*>(rt);
    }

    const T* launder(const raw_type* rt) const {
        return reinterpret_cast<const T*>(rt);
    }

    void destroy(raw_type* rt){
        launder(rt)->~T();
    }
};

#endif