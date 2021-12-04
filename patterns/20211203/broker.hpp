//辅助Actor编程模型的消息机制
// - 支持发布-订阅模式
// - 支持请求-回复模式
// - 支持组合式设计
//生命周期管理启发自 http://nercury.github.io/c++/interesting/2016/02/22/weak_ptr-and-event-cleanup.html
//
//消息中间人broker可同时作为消息发布者publisher和请求发出者client
//作为publisher时,使用方式如下
//  1. 通过publish接口发送消息,注意只接收单个参数,发出时为const&
//  2. 要订阅消息可以使用以下接口
//     2.1 subscribe(publish.channel<MessageType>,callback)
//     2.2 subscriber(YourClass).subscribe(publisher.channel<MessageType>,[callback])
//         2.2.1 callback的第一个参数为YourClass*
//         2.2.2 不指定callback时,使用YourClass::on(MessageType&&)作为默认callback
//         2.2.3 也有subscribe<MessageType>(publisher,[callback])形式可用,且使用on函数时可以接受MessageType序列
// 
//作为client时,使用方式如下
//  1. 通过request接口请求回复,请求可附带参数(const &),回复支持两种指定方式
//	  1.1 request([params],R& result)->void :需提供result引用来写入回复
//    1.2 request([params],Op& op)->op.value() :op需要提供alloc接口提供回复引用供写入,以及value接口来返回结果
//  2. 需要回复消息可以使用以下接口
//    2.1 bind(client.endpoint<[ArgType],ResultType>(),callback)
//    2.2 binder(YourClass).bind(client.endpoint<[ArgType],ResultType>(),[callback])
//       2.2.1 callback的第一个参数为YourClass*
//       2.2.2 不指定callback时,使用YourClass::reply(ArgType&&,ResultType&)作为默认callback
//       2.2.3 也有bind<[ArgType],ResultType>(client,[classback])形式可用
//
// 一旦与broker建立关联,就会提供broker::action_stub给对方作为存根,务必正确处理存根
// 存根具有release接口用来释放关联,析构时也会自动调用
// 当使用YourClass和broker关联时,存根建议作为类成员变量处理,以保证和YourClass生命周期一致

#pragma once
#include <type_traits>
#include <string_view>
#include <functional>
#include <memory>
#include <vector>
#include <cassert>
#include <algorithm>

//编译期类型ID机制可参考 https://stackoverflow.com/a/56600402
using type_code = std::string_view;

template<typename... Ts>
constexpr type_code type_code_of() noexcept {
#ifdef __clang__
	return __PRETTY_FUNCTION__;
#elif defined(__GNUC__)
	return __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
	return __FUNCSIG__;
#else
#error "Unsupported compiler"
#endif
}

class broker {

	struct message_handler_base {
		template<typename T>
		const void* address_of(const T& obj) {
			if constexpr (!std::is_polymorphic_v<T> || std::is_final_v<T>) {
				return &obj;
			}
			else
				return dynamic_cast<const void*>(&obj);
		}

		template<typename T>
		void on(T const& msg) {
			handle_impl(address_of(msg), nullptr, type_code_of<T, void>());
		}

		template<typename T, typename R>
		void reply(T const& msg, R& result) {
			handle_impl(address_of(msg), const_cast<void*>(address_of(result)), type_code_of<T, R>());
		}

		template<typename R>
		void reply(R& result) {
			handle_impl(nullptr, const_cast<void*>(address_of(result)), type_code_of<void, R>());
		}
	protected:
		virtual void handle_impl(const void* msg, void* result, type_code code) = 0;
	};

	template<typename Fn, typename T, typename R>
	class message_handler final :public message_handler_base {
		Fn m_action;
	public:
		explicit message_handler(Fn&& fn) :m_action(std::move(fn)) {};
	protected:
		void handle_impl(const void* msg, void* result, type_code code) override final {
			constexpr auto require_code = type_code_of<T, R>();
			assert(code == require_code);
			if constexpr (std::is_same_v<T, void>) {
				m_action(*static_cast<R*>(result));
			}
			else if constexpr (std::is_same_v<R, void>) {
				m_action(*static_cast<const T*>(msg));
			}
			else {
				m_action(*static_cast<const T*>(msg), *static_cast<R*>(result));
			}
		}
	};

	struct handler_stub {
		type_code code;
		std::weak_ptr<message_handler_base> handler;
	};

	std::vector<handler_stub> m_stubs;
	unsigned                  m_concurrent_count{};

	template<std::size_t I>
	struct priority_tag :priority_tag<I - 1> {};

	template<>
	struct priority_tag<0> {};

	template<typename Fn>
	void loop(type_code code, Fn&& fn) {
		m_concurrent_count++;
		try {
			std::size_t i = 0;
			while (i < m_stubs.size()) {
				auto&& o = m_stubs[i++];
				if (o.code != code) continue;
				if (auto&& h = o.handler.lock()) {
					fn(*h.get());
				}
			}
			m_concurrent_count--;
		}
		catch (...) {
			m_concurrent_count--;
			throw;
		}
	}

	void tidy() {
		if (0 != m_concurrent_count)
			return;
		m_stubs.erase(std::remove_if(m_stubs.begin(), m_stubs.end(), [](auto&& o) { return o.handler.expired(); }), m_stubs.end());
	}

	template<typename T, typename Op>
	auto request_impl(T const& msg, Op& op, priority_tag<1>) ->decltype(op.value()) {
		using R = std::remove_reference_t<decltype(op.alloc())>;
		loop(type_code_of<T, R>(), [&](auto&& h) { h.reply(msg, op.alloc()); });
		return op.value();
	}

	template<typename T, typename R>
	auto request_impl(T const& msg, R& result, priority_tag<0>) ->decltype(void()) {
		loop(type_code_of<T, R>(), [&](auto&& h) { h.reply(msg, result); });
	}

	template<typename Op>
	auto request_impl(Op& op, priority_tag<1>)->decltype(op.value()) {
		using R = std::remove_reference_t<decltype(op.alloc())>;
		loop(type_code_of<void, R>(), [&](auto&& h) { h.reply(op.alloc()); });
		return op.value();
	}

	template<typename R>
	auto request_impl(R& result, priority_tag<0>) ->decltype(void()) {
		loop(type_code_of<void, R>(), [&](auto&& h) { h.reply(result); });
	}
public:
	using action_stub_unit = std::function<void()>;

	broker() = default;

	template<typename T>
	void publish(T const& msg) {
		loop(type_code_of<T, void>(), [&](auto&& h) { h.on(msg); });
	}

	template<typename T, typename R>
	decltype(auto) request(const T& msg, R& result) {
		return request_impl(msg, result, priority_tag<2>{});
	}

	template<typename R>
	decltype(auto) request(R& result) {
		return request_impl(result, priority_tag<2>{});
	}

	template<typename T, typename R>
	struct port_stub {
		broker* owner{};

		template<typename Fn>
		action_stub_unit bind(Fn&& fn) {
			assert(owner != nullptr);
			owner->tidy();
			auto handler = std::make_shared<message_handler<Fn, T, R>>(std::move(fn));
			owner->m_stubs.emplace_back(handler_stub{ type_code_of<T,R>(),handler });
			return[h = std::move(handler)](){};
		}
	};

	template<typename T>
	using channel_stub = port_stub<T, void>;

	template<typename T, typename R>
	using endpoint_stub = port_stub<T, R>;

	template<typename T>
	channel_stub<T> channel() noexcept {
		static_assert(!std::is_same_v<T, void>, "message type T cannot be void");
		return { this };
	}

	template<typename T, typename R>
	endpoint_stub<T, R> endpoint() noexcept {
		static_assert(!std::is_same_v<R, void>, "response type R cannot be void");
		return { this };
	}
public:
	class action_stub {
	public:
		action_stub() = default;
		action_stub(action_stub_unit&& stub) {
			m_actions.emplace_back(std::move(stub));
		}
		action_stub(std::vector<action_stub_unit>&& stubs) :m_actions(std::move(stubs)) {};

		action_stub(action_stub const&) = delete;
		action_stub& operator=(action_stub const&) = delete;

		action_stub(action_stub&& other) noexcept :m_actions(std::move(other.m_actions)) {};
		action_stub& operator=(action_stub&& other) noexcept {
			if (std::addressof(other) != this) {
				release();
				m_actions = std::move(other.m_actions);
			}
			return *this;
		}

		~action_stub() noexcept {
			try { release(); }
			catch (...) {};
		}

		action_stub& operator+=(action_stub_unit&& stub) noexcept {
			m_actions.emplace_back(std::move(stub));
			return *this;
		}

		action_stub& operator+=(action_stub&& other) noexcept {
			if (std::addressof(other) != this) {
				m_actions.reserve(m_actions.size() + other.m_actions.size());
				for (auto& stub : other.m_actions) {
					m_actions.emplace_back(std::move(stub));
				}
				other.m_actions.clear();
			}
			return *this;
		}

		void release() {
			for (auto& h : m_actions) {
				if (h) h();
			}
			m_actions.clear();
		}
	private:
		std::vector<action_stub_unit> m_actions;
	};
};

template<typename T, typename E = void>
struct subscribe_helper;

template<typename T>
struct subscribe_helper<broker::channel_stub<T>> {
	template<typename Fn>
	static broker::action_stub_unit subscribe(broker::channel_stub<T> channel, Fn&& fn) {
		return channel.bind(std::move(fn));
	}
};

template<typename T>
class subscriber {
	T* m_obj{};
	broker::action_stub m_stub;
public:
	explicit subscriber(T& obj) :m_obj(std::addressof(obj)) {};

	template<typename U, typename Fn>
	subscriber& subscribe(U& source, Fn&& fn) {
		m_stub += subscribe_helper<std::decay_t<U>>::subscribe(source,
			[obj = m_obj, h = std::move(fn)](auto arg){ std::invoke(h, obj, arg); }
		);
		return *this;
	}

	template<typename U>
	subscriber& subscribe(U& source) {
		m_stub += subscribe_helper<std::decay_t<U>>::subscribe(source,
			[obj = m_obj](auto& arg) { obj->on(arg); }
		);
		return *this;
	}

	template<typename U, typename Fn>
	subscriber& subscribe(broker& o, Fn&& fn) {
		return subscribe(o.channel<U>(), std::move(fn));
	}

	template<typename... Us>
	subscriber& subscribe(broker& o) {
		(subscribe(o.channel<Us>()), ...);
		return *this;
	}

	operator broker::action_stub() noexcept {
		return broker::action_stub{ std::move(m_stub) };
	}
};

template<typename T>
subscriber(T& obj)->subscriber<T>;

template<typename T, typename Fn>
broker::action_stub subscribe(T& source, Fn&& fn) {
	return subscribe_helper<std::decay_t<T>>::subscribe(source, std::move(fn));
}

template<typename T>
class binder {
	T* m_obj{};
	broker::action_stub m_stub;
public:
	explicit binder(T& obj) :m_obj(std::addressof(obj)) {};

	template<typename U, typename R, typename Fn>
	binder& bind(broker::endpoint_stub<U, R>& ep, Fn&& fn) {
		if constexpr (std::is_same_v<U, void>) {
			m_stub += ep.bind([obj = m_obj, h = std::move(fn)](auto&& result){ std::invoke(h, obj, result); });
		}
		else
		{
			m_stub += ep.bind([obj = m_obj, h = std::move(fn)](auto&& arg, auto&& result){ std::invoke(h, obj, arg, result); });
		}
		return *this;
	}

	template<typename U, typename R>
	binder& bind(broker::endpoint_stub<U, R>& ep) {
		if constexpr (std::is_same_v<U, void>) {
			m_stub += ep.bind([obj = m_obj](auto&& result) { obj->reply(result); });
		}
		else
		{
			m_stub += ep.bind([obj = m_obj](auto&& arg, auto&& result) { obj->reply(arg, result); });
		}
		return *this;
	}

	template<typename U, typename R, typename Fn>
	binder& bind(broker& o, Fn&& fn) {
		return bind(o.endpoint<U, R>(), std::move(fn));
	}

	template<typename U, typename R>
	binder& bind(broker& o) {
		return bind(o.endpoint<U, R>());
	}

	operator broker::action_stub() noexcept {
		return broker::action_stub{ std::move(m_stub) };
	}
};

template<typename T>
binder(T& obj)->binder<T>;

template<typename T, typename R, typename Fn>
broker::action_stub bind(broker::endpoint_stub<T, R>& ep, Fn&& fn) {
	return ep.bind(std::move(fn));
}

template<typename R>
struct response {
	std::vector<R> results;

	R& alloc() {
		results.emplace_back(R{});
		return results.back();
	}

	std::vector<R> value() noexcept {
		return std::move(results);
	}
};
