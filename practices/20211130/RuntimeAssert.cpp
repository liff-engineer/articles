#include <cassert>
#include <string>
#include <sstream>
#include <Windows.h>
#include <stdexcept>

//计算宏参数个数
#define VFUNC_NARGS_IMPL_(_1, _2, _3, _4, _5, _6, _7, N, ...) N
#define VFUNC_NARGS_IMPL(args)   VFUNC_NARGS_IMPL_ args
#define VFUNC_NARGS(...)   VFUNC_NARGS_IMPL((__VA_ARGS__, 7, 6, 5, 4, 3, 2, 1))

//通用的可变参数版本宏
#define VFUNC_IMPL_(name,n)  name##n
#define VFUNC_IMPL(name,n)  VFUNC_IMPL_(name,n)
#define VFUNC_(name,n) VFUNC_IMPL(name,n)
#define VFUNC_EXPAND(x, y) x y 
#define VFUNC(func,...) VFUNC_EXPAND(VFUNC_(func,VFUNC_NARGS(__VA_ARGS__)) , (__VA_ARGS__))

struct Assert {
	const wchar_t* const file;
	unsigned line;
	const wchar_t* const function;
	const wchar_t* const message;
};

struct Hint {
	const wchar_t* const author;
	const wchar_t* const date;
	const wchar_t* const comment;
};

class RuntimeAssertError :public std::runtime_error
{
	Assert m_assert;
	Hint   m_hint;
public:
	explicit RuntimeAssertError(Assert const& as, Hint const& hint)
		:std::runtime_error("RuntimeAssertError"), m_assert(as), m_hint(hint) {};

	void Report() const noexcept {
		std::wstringstream os;
		os << "msg:" << m_assert.message << "\n";
		os << "file:" << m_assert.file << "\n";
		os << "line:" << m_assert.line << "\n";
		os << "func:" << m_assert.function << "\n";
		os << "author:" << m_hint.author << "\n";
		os << "date:" << m_hint.date << "\n";
		os << "comment:" << m_hint.comment << "\n";
		auto message = os.str();
		OutputDebugStringW(os.str().c_str());
	}
};

#define ASSERT_WIDE_(s) L ## s
#define ASSERT_WIDE(s) ASSERT_WIDE_(s)
#define EXPR_ASSERT(expr) {ASSERT_WIDE(__FILE__), (unsigned)(__LINE__),ASSERT_WIDE(__FUNCSIG__),ASSERT_WIDE(#expr)}

constexpr auto admin = Hint{ L"liff.engineer@gmail.com",L"2021-11-29",L"" };
#define ASSERT_ARGS1(expr) \
	if(!!(expr)) {		 \
		throw RuntimeAssertError(EXPR_ASSERT(expr),admin); \
	}

#define ASSERT_ARGS2(expr,hint) \
	if(!!(expr)) {		 \
		throw RuntimeAssertError(EXPR_ASSERT(expr),hint); \
	}

#define ASSERT_ARGS4(expr,author,date,comment) \
	if(!!(expr)) {		 \
		throw RuntimeAssertError(EXPR_ASSERT(expr),{author,date,comment}); \
	}

//可变参数宏定义
#define ASSERT(...)  VFUNC(ASSERT_ARGS,__VA_ARGS__)

constexpr auto liff_20211130 = Hint{ L"liff.engineer@gmail.com",L"2021/11/30",L"不满足前置条件" };

constexpr auto n1 = VFUNC_NARGS(1);
constexpr auto n2 = VFUNC_NARGS(1, 2);
int main(int argc, char** argv) {
	try {
		ASSERT(false);
		ASSERT(false == true, liff_20211130);
		ASSERT(true, Hint{L"liff.engineer",L"2021/11/30",L"立即模式"});
		ASSERT(1 == 1, L"liff.engineer", L"2021/11/30", L"测试条件");
	}
	catch (RuntimeAssertError const& e) {
		e.Report();
	}
	return 0;
}