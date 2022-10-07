// async-call-helper.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

struct asyn_call_token
{
	virtual ~asyn_call_token() = default;

	template <typename Cast>
	static inline auto from_context(void* context) noexcept
	{
		std::unique_ptr<asyn_call_token> token(reinterpret_cast<asyn_call_token*>(context));
		auto cast_ptr = static_cast<Cast*>(token->get_token());
		return cast_ptr;
	}
protected:
	virtual void* get_token() = 0;
};

template <typename Caller, typename ...IFaces>
class async_call_helper 
	: public IFaces...
{
public:
	using ThisType = typename async_call_helper<Caller, IFaces...>;
	using IFaces::IFaces...;

	async_call_helper() 
		: IFaces()... 
	{
		lifetime_ref = std::make_shared<auto_ref_holder>(*this);
	}
	~async_call_helper() 
	{

	}


	asyn_call_token* get_context()
	{
		struct special_token final
			: public asyn_call_token
		{
			special_token(std::weak_ptr<auto_ref_holder> ref_) 
				: ref(ref_) {}
			~special_token() = default;

			void* get_token() override {
				auto sref = ref.lock();
				// at this pointer either we have 
				// the parent[ThisType] pointer or nullptr so we can safely operate over the pointer
				return (sref) ? sref->get_this() : nullptr;
			}
		private:
			std::weak_ptr<auto_ref_holder> ref;
		};
		return new special_token(weak_ref());
	}

protected:
	Caller* parent() {
		return static_cast<Caller*>(this);
	}

	const Caller* parent() const {
		return static_cast<const Caller*>(this);
	}

private:
	friend struct auto_ref_holder;
	struct auto_ref_holder	
		: public std::enable_shared_from_this<auto_ref_holder>
	{
		explicit auto_ref_holder(ThisType& parent) 
			: ref(parent) {}

		Caller* get_this() {
			return ref.parent();
		}

		const Caller* get_this() const {
			return ref.parent();
		}
	private:
		ThisType& ref;
	};

	std::weak_ptr<auto_ref_holder> weak_ref() noexcept {
		return lifetime_ref;
	}

	std::weak_ptr<auto_ref_holder> weak_ref() const noexcept {
		return lifetime_ref;
	}

	std::shared_ptr<auto_ref_holder> lifetime_ref;
};


