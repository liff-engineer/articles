#include "Registry.hpp"
#pragma once
namespace abc
{
    template<typename T>
    struct FactoryBase::Argument :public FactoryBase::IArgument {
        Argument() = default;
        explicit Argument(const T& obj) :vp(std::addressof(obj)) {};

        static std::unique_ptr<IArgument> Default() { return nullptr; }
        
        const T* vp{};
    };

    template<>
    struct FactoryBase::Argument<void> :public FactoryBase::IArgument {
        Argument() = default;
        static std::unique_ptr<IArgument> Default() {
            return std::make_unique<Argument<void>>();
        }
    };

    template<typename T>
    struct FactoryBase::ArgumentHolder final :public FactoryBase::Argument<T> {
        T v;
        template<typename... Args>
        explicit ArgumentHolder(Args&&... args)
            :Argument<T>(), v(std::forward<Args>(args)...) {
            vp = std::addressof(v);
        }
    };

	template<typename I>
	struct FactoryBase::iGen {
		template<typename T>
		std::unique_ptr<I> Make(const Argument<void>& arg) {
			return MakeImpl<T>(std::is_base_of<I, T>{});
		}

		template<typename T, typename Arg>
		std::unique_ptr<I> Make(const Argument<Arg>& arg) {
			return MakeImpl<T>(std::is_base_of<I, T>{}, * arg.vp);
		}
	protected:
		template<typename T, typename... Args>
		std::unique_ptr<I> MakeImpl(std::true_type, Args&&... args) {
			return std::make_unique<T>(std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		std::unique_ptr<I> MakeImpl(std::false_type, Args&&... args) {
			//当类型T没有派生自接口I时,通过GenInject获取要构建的类型
			return std::make_unique<FactoryInject<T, I>::type>(std::forward<Args>(args)...);
		}
	};

	template<typename I>
	template<typename Arg>
	inline std::unique_ptr<I> FactoryBase::ObjGen<I>::Make(const Argument<Arg>& v) const
    {
        static const auto code = typeid(Arg).name();
        if (std::strcmp(code, argCode) != 0) return {};
        return op(v);
    }

    template<typename I>
    inline std::unique_ptr<I> FactoryBase::ObjGen<I>::Make(const Argument<void>& v) const
    {
        if (arg) return op(*arg.get());
        return {};
    }

    template<typename I>
    template<typename T, typename Arg>
    inline FactoryBase::ObjGen<I> FactoryBase::ObjGen<I>::Create()
    {
        return ObjGen<I>{ typeid(Arg).name(), Argument<Arg>::Default(),
            [](const IArgument& arg)->std::unique_ptr<I> {
                return iGen<I>{}.Make<T>(*static_cast<const Argument<Arg>*>(&arg));
            }
        };
    }
}
