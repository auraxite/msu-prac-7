#pragma once

#include <atomic>
#include <compare>
#include <concepts>
#include <cstddef>
#include <utility>

namespace detail {

// Общая часть блока управления: счётчик ссылок и виртуальный деструктор,
// чтобы SharedPtr<Base> корректно удалял объект Derived.
class ControlBlockBase {
public:
    virtual ~ControlBlockBase() = default;

    void add_ref() noexcept { refs_.fetch_add(1, std::memory_order_relaxed); }

    // true, если отпущена последняя ссылка
    bool release() noexcept { return refs_.fetch_sub(1, std::memory_order_acq_rel) == 1; }

    std::size_t use_count() const noexcept { return refs_.load(std::memory_order_relaxed); }

private:
    std::atomic<std::size_t> refs_{1};
};

// Помнит исходный тип U, поэтому удаляет объект через правильный деструктор.
template <typename U>
class ControlBlock final : public ControlBlockBase {
public:
    explicit ControlBlock(U* ptr) noexcept : ptr_(ptr) {}
    ~ControlBlock() override { delete ptr_; }

private:
    U* ptr_;
};

}  // namespace detail

template <typename T>
class SharedPtr {
public:
    using element_type = T;

    constexpr SharedPtr() noexcept = default;
    constexpr SharedPtr(std::nullptr_t) noexcept {}

    template <typename U>
        requires std::convertible_to<U*, T*>
    explicit SharedPtr(U* ptr) : ptr_(ptr) {
        if (ptr == nullptr) {
            return;
        }
        try {
            cb_ = new detail::ControlBlock<U>(ptr);
        } catch (...) {
            delete ptr;
            throw;
        }
    }

    SharedPtr(const SharedPtr& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) { acquire(); }

    template <typename U>
        requires std::convertible_to<U*, T*>
    SharedPtr(const SharedPtr<U>& other) noexcept : ptr_(other.ptr_), cb_(other.cb_) {
        acquire();
    }

    SharedPtr(SharedPtr&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)), cb_(std::exchange(other.cb_, nullptr)) {}

    template <typename U>
        requires std::convertible_to<U*, T*>
    SharedPtr(SharedPtr<U>&& other) noexcept
        : ptr_(std::exchange(other.ptr_, nullptr)), cb_(std::exchange(other.cb_, nullptr)) {}

    ~SharedPtr() { release(); }

    // Copy-and-swap: безопасно при самоприсваивании
    SharedPtr& operator=(const SharedPtr& other) noexcept {
        SharedPtr(other).swap(*this);
        return *this;
    }

    template <typename U>
        requires std::convertible_to<U*, T*>
    SharedPtr& operator=(const SharedPtr<U>& other) noexcept {
        SharedPtr(other).swap(*this);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) noexcept {
        SharedPtr(std::move(other)).swap(*this);
        return *this;
    }

    template <typename U>
        requires std::convertible_to<U*, T*>
    SharedPtr& operator=(SharedPtr<U>&& other) noexcept {
        SharedPtr(std::move(other)).swap(*this);
        return *this;
    }

    SharedPtr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    void reset() noexcept { SharedPtr().swap(*this); }

    template <typename U>
        requires std::convertible_to<U*, T*>
    void reset(U* ptr) {
        SharedPtr(ptr).swap(*this);
    }

    void swap(SharedPtr& other) noexcept {
        std::swap(ptr_, other.ptr_);
        std::swap(cb_, other.cb_);
    }

    T* get() const noexcept { return ptr_; }
    T& operator*() const noexcept { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }

    std::size_t use_count() const noexcept { return cb_ != nullptr ? cb_->use_count() : 0; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    template <typename U>
    friend class SharedPtr;

    void acquire() noexcept {
        if (cb_ != nullptr) {
            cb_->add_ref();
        }
    }

    void release() noexcept {
        if (cb_ != nullptr && cb_->release()) {
            delete cb_;
        }
    }

    T* ptr_ = nullptr;
    detail::ControlBlockBase* cb_ = nullptr;
};

template <typename T>
void swap(SharedPtr<T>& lhs, SharedPtr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

template <typename T, typename U>
bool operator==(const SharedPtr<T>& lhs, const SharedPtr<U>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <typename T, typename U>
std::strong_ordering operator<=>(const SharedPtr<T>& lhs, const SharedPtr<U>& rhs) noexcept {
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
