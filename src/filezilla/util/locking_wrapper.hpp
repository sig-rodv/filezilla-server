#ifndef FZ_UTIL_LOCKING_WRAPPER_HPP
#define FZ_UTIL_LOCKING_WRAPPER_HPP

#include <libfilezilla/mutex.hpp>
#include "../util/traits.hpp"

namespace fz::util {

template <typename T, typename Mutex = fz::mutex>
class locking_wrapper;

template <typename T, typename SFINAE = void>
class locked_proxy
{
public:
	using value_type = T;

	locked_proxy(T *value = nullptr, fz::mutex *mutex = nullptr)
		: value_(value)
	{
		if (value_)
			mutex_ = mutex;
		else {
			if (mutex)
				mutex->unlock();

			mutex_ = nullptr;
		}
	}

	locked_proxy(locked_proxy &&rhs) noexcept
		: locked_proxy(rhs.value_, rhs.mutex_)
	{
		rhs.mutex_ = nullptr;
	}

	locked_proxy(const locked_proxy &) = delete;
	locked_proxy operator =(const locked_proxy &) = delete;
	locked_proxy &operator=(locked_proxy &&rhs) = delete;

	~locked_proxy()
	{
		if (mutex_)
			mutex_->unlock();
	}

	T &operator *()
	{
		return *value_;
	}

	T *operator ->()
	{
		return value_;
	}

	const T *operator ->() const
	{
		return value_;
	}

	template <typename... Args>
	auto operator()(Args &&... args) -> decltype(value_(std::forward<Args>(args)...))
	{
		return (*value_)(std::forward<Args>(args)...);
	}

	explicit operator bool() const
	{
		return value_ != nullptr;
	}

	T *get()
	{
		return value_;
	}

	template <typename A, typename B>
	friend locked_proxy<A> static_locked_proxy_cast(locked_proxy<B> &&b);

	template <typename A, typename B>
	friend locked_proxy<A> dynamic_locked_proxy_cast(locked_proxy<B> &&b);

	template <typename A, typename B>
	friend locked_proxy<A> reintepret_locked_proxy_cast(locked_proxy<B> &&b);

private:
	T *value_;
	fz::mutex *mutex_;
};

FZ_UTIL_TRAITS_MAKE_ACCESS_TEMPLATE(locked_proxy)

template <typename A, typename B>
locked_proxy<A> static_locked_proxy_cast(locked_proxy<B> &&b)
{
	locked_proxy<A> ret {static_cast<A*>(b.value_), b.mutex_};

	b.mutex_ = nullptr;

	return ret;
}

template <typename A, typename B>
locked_proxy<A> dynamic_locked_proxy_cast(locked_proxy<B> &&b)
{
	locked_proxy<A> ret {dynamic_cast<A*>(b.value_), b.mutex_};

	b.mutex_ = nullptr;

	return ret;
}

template <typename A, typename B>
locked_proxy<A> reintepret_locked_proxy_cast(locked_proxy<B> &&b)
{
	locked_proxy<A> ret {reinterpret_cast<A*>(b.value_), b.mutex_};

	b.mutex_ = nullptr;

	return ret;
}

template <typename Proxy>
class locked_proxy<Proxy, std::enable_if_t<trait::is_locked_proxy_v<Proxy>>>
{
public:
	using value_type = typename Proxy::value_type;

	locked_proxy()
		: proxy_()
		, mutex_(nullptr)
	{}

	locked_proxy(Proxy &&proxy, fz::mutex *mutex)
		: proxy_(std::move(proxy))
		, mutex_(mutex)

	{}

	locked_proxy(locked_proxy &&rhs) noexcept
		: proxy_(std::move(rhs.proxy_))
		, mutex_(rhs.mutex_)
	{
		rhs.mutex_ = nullptr;
	}

	locked_proxy(const locked_proxy &) = delete;
	locked_proxy operator =(const locked_proxy &) = delete;
	locked_proxy &operator=(locked_proxy &&rhs) = delete;

	~locked_proxy()
	{
		if (mutex_)
			mutex_->unlock();
	}

	value_type &operator *()
	{
		return *proxy_;
	}

	value_type *operator ->()
	{
		return &*proxy_;
	}

	template <typename... Args>
	auto operator()(Args &&... args) -> decltype(value_(std::forward<Args>(args)...))
	{
		return (*proxy_)(std::forward<Args>(args)...);
	}

	explicit operator bool() const
	{
		return bool(proxy_);
	}

	value_type *get()
	{
		return proxy_.get();
	}

private:
	Proxy proxy_;
	fz::mutex *mutex_;
};

template <typename T>
class locking_wrapper_interface
{
public:
	using proxy = locked_proxy<T>;

	virtual ~locking_wrapper_interface() = default;
	virtual proxy lock() = 0;
};

template <typename T>
class locking_wrapper<T, fz::mutex>: public locking_wrapper_interface<T>
{
public:
	template <typename ...Args>
	locking_wrapper(Args &&... args)
		: value_(std::forward<Args>(args)...)
	{}

	template <typename ...Args>
	locking_wrapper(std::in_place_t, Args &&... args)
		: value_{std::forward<Args>(args)...}
	{}

	locked_proxy<T> lock() override
	{
		mutex_.lock();

		return {&value_, &mutex_};
	}

private:
	T value_;
	fz::mutex mutex_{true};
};

template <typename T>
class locking_wrapper<T, fz::mutex&>: public locking_wrapper_interface<T>
{
public:
	template <typename ...Args>
	locking_wrapper(fz::mutex &mutex, Args &&... args)
		: mutex_(mutex)
		, value_(std::forward<Args>(args)...)
	{}

	template <typename ...Args>
	locking_wrapper(fz::mutex &mutex, std::in_place_t, Args &&... args)
		: mutex_(mutex)
		, value_{std::forward<Args>(args)...}
	{}

	locked_proxy<T> lock() override
	{
		mutex_.lock();

		return {&value_, &mutex_};
	}

private:
	fz::mutex &mutex_;
	T value_;
};

template <typename T>
class locking_wrapper<T, void>: public locking_wrapper_interface<T>
{
public:
	template <typename ...Args>
	locking_wrapper(Args &&... args)
		: value_(std::forward<Args>(args)...)
	{}

	template <typename ...Args>
	locking_wrapper(std::in_place_t, Args &&... args)
		: value_{std::forward<Args>(args)...}
	{}

	locked_proxy<T> lock() override
	{
		return {&value_, nullptr};
	}

private:
	T value_;
};

template <typename T>
class locking_wrapper<T&, fz::mutex>: public locking_wrapper_interface<T>
{
public:
	locking_wrapper(T &value)
		: value_(value)
	{}

	locked_proxy<T> lock() override
	{
		mutex_.lock();

		return {&value_, &mutex_};
	}

private:
	fz::mutex mutex_{true};
	T &value_;
};

template <typename T>
class locking_wrapper<T&, fz::mutex&>: public locking_wrapper_interface<T>
{
public:
	locking_wrapper(fz::mutex &mutex, T &value)
		: mutex_(mutex)
		, value_(value)
	{}

	locked_proxy<T> lock() override
	{
		mutex_.lock();

		return {&value_, &mutex_};
	}

private:
	fz::mutex &mutex_;
	T &value_;
};

template <typename T>
class locking_wrapper<T&, void>: public locking_wrapper_interface<T>
{
public:
	locking_wrapper(T &value)
		: value_(value)
	{}

	locked_proxy<T> lock() override
	{
		return {&value_, nullptr};
	}

private:
	T &value_;
};

}

#endif // FZ_UTIL_LOCKING_WRAPPER_HPP
