/// 设计中通常用到某对象发生变化,要同步刷新其它对象的情况
/// https://en.wikipedia.org/wiki/Observer_pattern
/// 解决这个问题一般用观察者模式Observer,这就带来以下问题:
/// - 多种Subject定义
/// - 多种Observer接口定义
/// - Observer需要注册到Subject上,并在合适时机移除
/// 
/// 考虑到这种场景通常用来保证某个模型文档的内部一致性
/// 采用如下思路设计:
/// 1. 将Subject发出的消息全部用普通的结构体等类型表示 
/// 2. 提供通用Notifyer,能够发送任何消息类型
/// 3. 如果某种对象类型需要观察特定消息类型,则注册一个函数,
///    以模型、消息为参数,当通知发出,会调用该函数来处理
/// 通过这种方式:
/// - 无需接口类定义和派生
/// - 无需单独的观察者注册、移除
/// - 不暂存观察者信息(避免观察者新增、销毁时进行处理),即时查询
/// 
/// 另外,无论是变更消息的发出、还是更新处理都可以放在对象外部实现

#include <functional>
#include <string>
#include <unordered_map>
#include <cassert>

/// @brief 观察者,其实现应为仿函数形式void operator()(const Owner&,const S&)
/// @tparam Owner  观察方的拥有方,用来查找特定类型的观察者 
/// @tparam T 观察方,希望能够响应消息S
/// @tparam S Subject发出的消息类型
/// @tparam E 观察者的偏特化支持
template<typename Owner, typename T, typename S, typename E = void>
struct Observer;

/// @brief  Subject聚合(可发送任意消息,引发观察者同步更新)
/// @tparam Owner 持有方(该设施用来保证其内部一致性)
template<typename Owner>
class Notifyer {
    struct IChannel {
        virtual ~IChannel() = default;
    };

    template<typename E>
    struct SubjectChannel final :public IChannel {
        std::unordered_map<std::string, std::function<void(const Owner&, const E&)>>
            observers;

        void notify(const Owner& owner, const E& e) const {
            for (auto&& [k, observer] : observers) {
                observer(owner, e);
            }
        }

        template<typename Fn>
        void emplace(std::string&& key, Fn&& observer) {
            observers[key] = std::move(observer);
        }
    };
    std::unordered_map<std::string, std::unique_ptr<IChannel>> m_channels;
public:
    template<typename E>
    void notify(const Owner& owner, const E& e) const {
        if (auto it = m_channels.find(typeid(E).name()); it != m_channels.end()) {
            if (auto subject = dynamic_cast<SubjectChannel<E>*>(it->second.get())) {
                subject->notify(owner, e);
            }
        }
    }

    template<typename T, typename E, typename... Args>
    void emplaceObserver(Args&&... args) {
        std::string tCode = typeid(T).name();
        std::string eCode = typeid(E).name();
        if (auto it = m_channels.find(eCode); it != m_channels.end()) {
            if (auto subject = dynamic_cast<SubjectChannel<E>*>(it->second.get())) {
                subject->emplace(std::move(tCode),
                    Observer<Owner, T, E>{std::forward<Args>(args)...});
            }
            else {
                assert(false);//应该是之前将E注册错了
            }
        }
        else
        {
            auto subject = std::make_unique<SubjectChannel<E>>();
            subject->emplace(std::move(tCode),
                Observer<Owner, T, E>{std::forward<Args>(args)...});
            m_channels[eCode] = std::move(subject);
        }
    }
};
