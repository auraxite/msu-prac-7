#pragma once

#include <atomic>
#include <compare>
#include <cstddef>
#include <utility>

template <typename T>
class SharedPtr {
public:
	constexpr SharedPtr() noexcept = default;
	constexpr SharedPtr(std::nullptr_t) noexcept {}

	explicit SharedPtr(T* ptr) : ptr_(ptr) {
		if (ptr == nullptr) {
			return;
		}
		try {
			count_ = new std::atomic<std::size_t>(1);
		} catch (...) {
			delete ptr;
			throw;
		}
	}

	SharedPtr(const SharedPtr& other) noexcept : ptr_(other.ptr_), count_(other.count_) {
		if (count_ != nullptr) {
			count_->fetch_add(1, std::memory_order_relaxed);
		}
	}

	SharedPtr(SharedPtr&& other) noexcept
		: ptr_(std::exchange(other.ptr_, nullptr)), count_(std::exchange(other.count_, nullptr)) {}

	~SharedPtr() {
		if (count_ != nullptr && count_->fetch_sub(1, std::memory_order_acq_rel) == 1) {
			delete ptr_;
			delete count_;
		}
	}

	SharedPtr& operator=(const SharedPtr& other) noexcept {
		SharedPtr(other).swap(*this);
		return *this;
	}

	SharedPtr& operator=(SharedPtr&& other) noexcept {
		SharedPtr(std::move(other)).swap(*this);
		return *this;
	}

	SharedPtr& operator=(std::nullptr_t) noexcept {
		reset();
		return *this;
	}

	void reset() noexcept { SharedPtr().swap(*this); }

	void swap(SharedPtr& other) noexcept {
		std::swap(ptr_, other.ptr_);
		std::swap(count_, other.count_);
	}

	T* get() const noexcept { return ptr_; }
	T& operator*() const noexcept { return *ptr_; }
	T* operator->() const noexcept { return ptr_; }

	std::size_t use_count() const noexcept { return count_ != nullptr ? count_->load(std::memory_order_relaxed) : 0; }
	explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
	T* ptr_ = nullptr;
	std::atomic<std::size_t>* count_ = nullptr;
};

template <typename T>
void swap(SharedPtr<T>& lhs, SharedPtr<T>& rhs) noexcept {
	lhs.swap(rhs);
}

template <typename T>
bool operator==(const SharedPtr<T>& lhs, const SharedPtr<T>& rhs) noexcept {
	return lhs.get() == rhs.get();
}

template <typename T>
std::strong_ordering operator<=>(const SharedPtr<T>& lhs, const SharedPtr<T>& rhs) noexcept {
	return std::compare_three_way{}(lhs.get(), rhs.get());
}

template <typename T>
bool operator==(const SharedPtr<T>& lhs, std::nullptr_t) noexcept {
	return !lhs;
}

template <typename T>
std::strong_ordering operator<=>(const SharedPtr<T>& lhs, std::nullptr_t) noexcept {
	return std::compare_three_way{}(lhs.get(), static_cast<T*>(nullptr));
}
