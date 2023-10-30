#pragma once

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
 private:
  friend class WeakPtr<T>;
  struct Counter {
    size_t shared_count_;
    size_t weak_count_;
  };
  T* ptr_;
  struct Counter* counter;

 public:
  SharedPtr() : ptr_(nullptr), counter(nullptr) {}

  SharedPtr(T* ptr) : ptr_(ptr), counter(new Counter) {
    counter->shared_count_ = ptr ? 1 : 0;
    counter->weak_count_ = 0;
  }

  SharedPtr(const SharedPtr& other) noexcept : counter(other.counter) {
    ptr_ = other.ptr_;
    ++counter->shared_count_;
  }

  SharedPtr(const SharedPtr&& other) noexcept
      : ptr_(other.ptr_), counter(other.counter) {
    other.counter = nullptr;
    other.ptr_ = nullptr;
  }

  template <typename U>
  SharedPtr(SharedPtr<U>&& other) noexcept
      : ptr_(other.ptr_), counter(other.counter) {
    other.counter = nullptr;
    other.ptr_ = nullptr;
  }

  SharedPtr(const WeakPtr<T>& other)
      : ptr_(other.ptr_), counter(other.counter) {
    if (other.Expired())
      throw std::bad_weak_ptr();
    if (counter)
      ++counter->shared_count_;
  }

  void DecreaseAndDelete() {
    if (!counter)
      return;
    if (counter->shared_count_ > 0)
      --counter->shared_count_;
    if (counter->shared_count_ == 0 && counter->weak_count_ == 0) {
      delete counter;
      delete ptr_;
      return;
    }
    if (counter->shared_count_ == 0) {
      delete ptr_;
    }
  }

  ~SharedPtr() { DecreaseAndDelete(); }

  SharedPtr& operator=(const SharedPtr& other) noexcept {
    if (this == &other)
      return *this;
    DecreaseAndDelete();
    counter = other.counter;
    ptr_ = other.ptr_;
    if (counter)
      ++counter->shared_count_;
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if (this == &other)
      return *this;
    DecreaseAndDelete();
    counter = other.counter;
    ptr_ = other.ptr_;
    other.counter = nullptr;
    other.ptr_ = nullptr;
    return *this;
  }

  void Reset(T* ptr = nullptr) {
    DecreaseAndDelete();
    ptr_ = ptr;
    counter = new (Counter);
    counter->shared_count_ = ptr ? 1 : 0;
    counter->weak_count_ = 0;
  }

  void Swap(SharedPtr<T>& other) {
    std::swap(counter, other.counter);
    std::swap(ptr_, other.ptr_);
  }

  T* Get() const { return ptr_; }

  size_t UseCount() const {
    if (!counter)
      return 0;
    return counter->shared_count_;
  }

  T& operator*() const { return *ptr_; }

  T* operator->() const { return ptr_; }

  operator bool() const { return counter && counter->shared_count_ != 0; }
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
class WeakPtr {
 private:
  friend class SharedPtr<T>;
  T* ptr_;
  struct SharedPtr<T>::Counter* counter;

 public:
  WeakPtr() : ptr_(nullptr), counter(nullptr) {}

  WeakPtr(T* ptr) : counter(new struct SharedPtr<T>::Counter) {
    ptr_ = ptr;
    counter->shared_count_ = 0;
    counter->weak_counter_ = ptr ? 1 : 0;
  }

  WeakPtr(const WeakPtr& other) noexcept : counter(other.counter) {
    if (counter)
      ++counter->weak_count_;
    ptr_ = other.ptr_;
  }

  WeakPtr(WeakPtr&& other) noexcept : counter(other.counter) {
    ptr_ = other.ptr_;
    other.counter = nullptr;
    other.ptr_ = nullptr;
  }

  WeakPtr(const SharedPtr<T>& other) noexcept : counter(other.counter) {
    ptr_ = other.ptr_;
    if (counter)
      ++counter->weak_count_;
  }

  template <typename U>
  WeakPtr(WeakPtr<U>&& other) {
    *this = other;
    ++counter->weak_count_;
  }

  void DecreaseAndDelete() {
    if (!counter)
      return;
    if (counter->weak_count_ > 0)
      --counter->weak_count_;
    if (counter->weak_count_ == 0 && counter->shared_count_ == 0) {
      delete counter;
    }
  }

  ~WeakPtr() { DecreaseAndDelete(); }

  WeakPtr& operator=(const WeakPtr& other) noexcept {
    if (this == &other)
      return *this;
    DecreaseAndDelete();
    counter = other.counter;
    ptr_ = other.ptr_;
    if (counter)
      ++counter->weak_count_;
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    if (this == &other)
      return *this;
    DecreaseAndDelete();
    counter = other.counter;
    ptr_ = other.ptr_;
    other.counter = nullptr;
    other.ptr_ = nullptr;
    return *this;
  }

  void Swap(WeakPtr<T>& other) {
    std::swap(ptr_, other.ptr_);
    std::swap(counter, other.counter);
  }

  void Reset() {
    if (!counter)
      return;
    --counter->weak_count_;
    ptr_ = nullptr;
    counter = nullptr;
  }

  size_t UseCount() const {
    if (!counter)
      return 0;
    return counter->shared_count_;
  }

  bool Expired() const {
    if (!counter)
      return true;
    return counter->shared_count_ == 0;
  }

  SharedPtr<T> Lock() const {
    if (Expired())
      return nullptr;
    SharedPtr<T> ptr(*this);
    return ptr;
  }
};