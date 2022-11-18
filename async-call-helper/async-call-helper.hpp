// async-call-helper.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>
#include <mutex>
#include <functional>

struct asyn_call_token
{
	virtual ~asyn_call_token() = default;

	template <typename Cast>
	static inline Cast* from_context(void* context) noexcept
	{
		std::unique_ptr<asyn_call_token> act_handle(reinterpret_cast<asyn_call_token*>(context));
		if (!act_handle) return nullptr;
		auto *cast_ptr = static_cast<Cast*>(act_handle->get_caller());
		return cast_ptr;
	}
protected:
	virtual void* get_caller() = 0;
};

template <typename Caller, typename ...IFaces>
class async_call_helper 
	: public IFaces...
{
public:
	using ThisType = async_call_helper<Caller, IFaces...>;
	// using IFaces::IFaces...;
	~async_call_helper() = default;
protected:
	async_call_helper() 
		: IFaces()... 
	{
		lifetime_ref = std::make_shared<auto_ref_holder>(parent());
	}

	void* get_context() const noexcept
	{
		struct special_token final
			: public asyn_call_token
		{
			special_token(std::weak_ptr<auto_ref_holder> ref_, std::mutex& guard_) 
				: ref(ref_) 
				, guard(guard_, std::defer_lock) {}
			
			~special_token() 
			{
				if (guard) {
					guard.unlock();
				}
			}

			void* get_caller() override {
				guard.lock();
				auto sref = ref.lock();
				// at this pointer either we have 
				// the parent[ThisType] pointer or nullptr so we can safely operate over the pointer
				return (sref) ? sref->get_parent() : nullptr;
			}
		private:
			std::weak_ptr<auto_ref_holder> ref;
			std::unique_lock<std::mutex> guard;
		};
		return new special_token(weak_ref(), guard);
	}
	template <typename ...Args>
	struct callback_context {
		void *context;
		void (*callback)(void*, Args...);
		void operator()(Args... args) noexcept {
			std::invoke(callback, context, std::forward<Args>(args)...);
		}
	};

	template <typename ...Args, typename Fn>
	callback_context<Args...> 
	get_context(Fn&& cb) noexcept 
	{
		struct trampoline_t final
			: public asyn_call_token
		{
			trampoline_t(std::weak_ptr<auto_ref_holder> ref_, 
						 std::mutex& guard_,
						 std::function<void(Args...)> callback_) 
				: ref(ref_) 
				, guard(guard_, std::defer_lock)
				, callback(std::move(callback_)) {}
			
			~trampoline_t() {
				if (guard) {
					guard.unlock();
				}
			}

			void* get_caller() override {
				guard.lock();
				auto sref = ref.lock();
				// at this pointer either we have 
				// the parent[ThisType] pointer or nullptr so we can safely operate over the pointer
				return (sref) ? sref->get_parent() : nullptr;
			}

			static inline void callback_handle(void* context, Args... args) {
				std::unique_ptr<trampoline_t> trampoline_ptr(reinterpret_cast<trampoline_t*>(context));
				if (!trampoline_ptr) return;
  			if (trampoline_ptr->get_caller()) {
					std::invoke(trampoline_ptr->callback, std::forward<Args>(args)...);
				}
			}
		private:
			std::weak_ptr<auto_ref_holder> ref;
			std::unique_lock<std::mutex> guard;
			std::function<void(Args...)> callback;
		};
		std::function<void(Args...)> callback = cb;
		return callback_context<Args...> {
			new trampoline_t(weak_ref(), guard, std::move(callback)),
			&trampoline_t::callback_handle
		};
	}

protected:

	Caller* parent() {
		return static_cast<Caller*>(this);
	}

	const Caller* parent() const {
		return static_cast<const Caller*>(this);
	}

	void set_deleted() noexcept {
		std::lock_guard<std::mutex> lock(guard);
		lifetime_ref.reset();
	}

private:
	friend struct auto_ref_holder;
	struct auto_ref_holder {
		explicit auto_ref_holder(Caller* caller_) : caller(caller_) {}
		Caller* get_parent() {return caller;}
		const Caller* get_parent() const {return caller;}
	private:
		Caller* caller;
	};

	std::weak_ptr<auto_ref_holder> weak_ref() noexcept {
		return lifetime_ref;
	}

	std::weak_ptr<auto_ref_holder> weak_ref() const noexcept {
		return lifetime_ref;
	}

	std::shared_ptr<auto_ref_holder> lifetime_ref;
	mutable std::mutex guard;
};


