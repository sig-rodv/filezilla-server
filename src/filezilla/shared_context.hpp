#ifndef FZ_SHARED_CONTEXT_HPP
#define FZ_SHARED_CONTEXT_HPP

#include <cassert>

#include <memory>

#include "util/locking_wrapper.hpp"

namespace fz {

template <typename T, typename Mutex = fz::mutex>
struct shared_context_data;

template <typename T,  typename Mutex = fz::mutex>
struct shared_context_base;

template <typename T, typename Mutex = fz::mutex>
struct shared_context;

template <typename T, typename Mutex = fz::mutex>
class enabled_for_shared_context;

template <typename T, typename Mutex>
struct shared_context_base
{
	util::locked_proxy<T> lock() const
	{
		if (data_) {
			data_->mutex_.lock();
			return {data_->v_, &data_->mutex_};
		}

		return { nullptr, nullptr };
	}

	shared_context_base() = default;
	shared_context_base(shared_context_base &&) = default;
	shared_context_base(const shared_context_base &) = default;

	shared_context_base &operator=(shared_context_base &&) = default;
	shared_context_base &operator=(const shared_context_base &) = default;

	explicit operator bool() const noexcept
	{
		if (data_) {
			scoped_lock lock(data_->mutex_);

			return data_->v_ != nullptr;
		}

		return false;
	}

	void detach() noexcept
	{
		data_.reset();
	}

protected:
	std::shared_ptr<shared_context_data<T, Mutex>> data_;

	void reset()
	{
		scoped_lock lock(data_->mutex_);
		data_->v_ = nullptr;
	}

	shared_context_base(std::shared_ptr<shared_context_data<T, Mutex>> && d)
		: data_(std::move(d))
	{}
};


template <typename T>
struct shared_context_data<T, fz::mutex>
{
	shared_context_data(T &v)
		: v_(&v)
	{}

	T *v_;
	mutable fz::mutex mutex_;
};

template <typename T>
struct shared_context_data<T, fz::mutex&>
{
	shared_context_data(T &v, fz::mutex &mutex)
		: v_(&v)
		, mutex_(mutex)
	{}

	T *v_;
	fz::mutex &mutex_;
};

template <typename T>
struct shared_context<T, fz::mutex> final: shared_context_base<T, fz::mutex>
{
	shared_context() = default;
	shared_context(shared_context &&) = default;
	shared_context(const shared_context &) = default;

	shared_context &operator=(shared_context &&) = default;
	shared_context &operator=(const shared_context &) = default;

private:
	friend enabled_for_shared_context<T, fz::mutex>;

	shared_context(T &v)
		: shared_context_base<T, fz::mutex>(std::make_shared<shared_context_data<T, fz::mutex>>(v))
	{}
};

template <typename T>
struct shared_context<T, fz::mutex&> final: shared_context_base<T, fz::mutex&>
{
	shared_context(shared_context &&) = default;
	shared_context(const shared_context &) = default;

	shared_context &operator=(shared_context &&) = default;
	shared_context &operator=(const shared_context &) = default;

private:
	friend enabled_for_shared_context<T, fz::mutex&>;

	shared_context(T &v, fz::mutex &mutex)
		: shared_context_base<T, fz::mutex&>(std::make_shared<shared_context_data<T, fz::mutex&>>(v, mutex))
	{}
};

template <typename T>
class enabled_for_shared_context<T, fz::mutex>
{
public:
	using shared_context_type = fz::shared_context<T, fz::mutex>;

	virtual ~enabled_for_shared_context() = 0;

	const shared_context_type &get_shared_context() const
	{
		return context_;
	}

protected:
	void stop_sharing_context()
	{
		context_.reset();
	}

	enabled_for_shared_context(T &v)
		: context_(v)
	{}

private:
	shared_context_type context_;
};

template <typename T>
class enabled_for_shared_context<T, fz::mutex&>
{
public:
	using shared_context_type = fz::shared_context<T, fz::mutex&>;

	virtual ~enabled_for_shared_context() = 0;

	const shared_context_type &get_shared_context() const
	{
		return context_;
	}

protected:
	void stop_sharing_context()
	{
		context_.reset();
	}

	enabled_for_shared_context(T &v, fz::mutex &mutex)
		: context_(v, mutex)
	{}

private:
	shared_context_type context_;
};

template <typename T>
enabled_for_shared_context<T, fz::mutex>::~enabled_for_shared_context()
{
	assert(!context_ && "You must invoke stop_sharing() from the most derived class' destructor.");
}

template <typename T>
enabled_for_shared_context<T, fz::mutex&>::~enabled_for_shared_context()
{
	assert(!context_ && "You must invoke stop_sharing() from the most derived class' destructor.");
}

}

#endif // FZ_SHARED_CONTEXT_HPP
