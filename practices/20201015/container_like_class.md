[TOC]

## 体验更好的'容器-like'类API设计与实现

对于开发者来讲,编码时使用的“心智模型”越简单,体验会越好,如何来理解呢?这里举个C++的例子来对比一下,假设有`Book`类,还有个`Booklist`类用来存储`Book`,并提供一些接口,通常会碰见这样设计的:

```C++
class Book{};

class Booklist{
public:
  	int   getCount();//个数
    Book* getItem(int index);
    //...
};
```

要知道通常C++开发者习惯使用**STL**提供的容器、迭代器和算法进行操作,本来遍历`Booklist`使用的方式是这样的:

```C++
Booklist list;
for(auto& book:list){
    //...
}
```

由于上述的`API`设计与C++开发者惯常的认知和操作方式的不同,会被强制扭成如下写法:

```C++
for(auto i = 0 ; i < list.getCount();i++){
    auto book = list.getItem(i);
    //...
}
```

更别说还有些Java等开发者转向C++时会设计如下接口:

```C++
class Booklist{
public:
    Book* first();
    Book* next();
    bool isDone();
};
```

然后使用者被迫以如下方式使用:

```C++
for(auto book = list.first();!list.isDone();book = list.next()){
    //...
}
```

当开发者碰到上述与语言惯用法不一致的各种API之后,会明显增加编码时的心智负担,体验非常差.



从开发者体验的角度看来,尤其是针对类似于容器的类API进行设计时,尽可能提供和语言习惯一致的API,是比较好的做法.这里从Python和C++两种语言来展示如何去做.

### Python中如何模拟容器类型

Python内置了`list`和`dict`两种数据类型,并在[Data model - Emulating container types](https://python.readthedocs.io/en/latest/reference/datamodel.html#emulating-container-types)中提供了关于如何模拟容器类型的要求,鉴于序列和映射的区别在于序列提供的是整数索引、映射使用的是`Key`,这里对类型的要求是一致的:

|                                      | 释义                             |
| ------------------------------------ | -------------------------------- |
| `object.__len__(self)`               | 容器大小                         |
| `object.__getitem__(self,key)`       | 根据`key`获取容器项              |
| `object.__setitem__(self,key,value)` | 根据`key`设置容器项              |
| `object.__delitem__(self,key)`       | 根据`key`删除容器项              |
| `object.__iter__(self)`              | 返回能够访问容器内容的迭代器对象 |

迭代器用来顺序访问容器内容,使得可以用在`for`和`in`中,其要求如下:

|                       | 释义                              |
| --------------------- | --------------------------------- |
| `iterator.__iter__()` | 返回迭代器自身                    |
| `iterator.__next__()` | 返回容器的下一项,无内容则抛出异常 |



假设有个对象`Item`,定义如下:

```C++
class Item:
    def __init__(self, name, price):
        self.name = name
        self.price = price
```

现在构造了一组`Item`,希望能够拿到这组`Item`里最低的`price`,以及是否`price`都一样,通常写法如下:

```python
items = [Item("1", 1), Item("2", 2), Item("3", 3)]

lowest_price = sys.float_info.max
all_equal = True
last_price = items[0]

for item in items:
    if item.price != last_price:
        all_equal = False
    if item.price < lowest_price:
        lowest_price = item.price

    last_price = item.price
```

这时想要将其封装成类`Items`:

```python
class Items:
    def __init__(self):
        self._items = []
        self.lowest_price = sys.float_info.max
        self.all_equal = True
        self.last_price = None

    def add_item(self, item):
        current_price = item.price
        if self.lowest_price > current_price:
            self.lowest_price = current_price
        if self.last_price is not None and self.last_price != current_price:
            self.all_equal = False
        self.last_price = current_price
        self._items.append(item)
```

这时就无法将`Items`作为容器遍历了,即如下语句:

```python
items = Items()

for item in items:
    #...   运行报错
```

如果需要遍历`Items`内部的值,可以为其提供`__iter__`:

```python
#迭代器类型
class ItemsIterator:
    def __init__(self, items):
        self._items = items
        self._index = 0
     
    def __iter__(self):
        return self

    def __next__(self):
        if self._index < len(self._items):
            result = self._items[self._index]
            self._index += 1
            return result
        raise StopIteration

class Items:

    def __iter__(self):
        #返回迭代器对象
        return ItemsIterator(self._items)
```

为了使其操作起来更像是`List`,则需要提供如下实现:

```python
class Items:
    def __len__(self):
        return len(self._items)

    def __getitem(self, key):
        return self._items[key]

    def __setitem(self, key, value):
        self._items[key] = value

    def __delitem(self, key):
        del (self._items[key])
```

### C++中如何自定义迭代器

在C++中(C++20提供了range,有了新的方式),**STL**抽象分为容器、迭代器、算法,其中迭代器是容器和算法之间的桥梁.共分为5种访问型迭代器:

| 分类                    | 能力                                                    |
| ----------------------- | ------------------------------------------------------- |
| `InputIterator`         | 能够读值,且只能单向递增                                 |
| `ForwardIterator`       | 具备`InputIterator`的能力,<br>并且可以单向`+n`          |
| `BidirectionalIterator` | 具备`ForwardIterator`的能力,<br>并且可以递减            |
| `RandomAccessIterator`  | 具备`BindirectionalIterator`的能力,<br>并且可以随机访问 |
| `ContiguousIterator`    | 具备`RandomAccessIterator`的能力,<br>并且内存连续       |

可以在[cppreference.com - Iterator library](https://en.cppreference.com/w/cpp/iterator)中查阅每种迭代器的规格要求.也可以搜索[Iterators: What Must Be Done?](https://infektor.net/posts/2018-11-03-iterators-what-must-be-done.html)来直接查阅每种迭代器具体要求.

这里以[gsl](https://github.com/microsoft/GSL/blob/master/include/gsl/span)中的`span`为例来讲解一下随机访问迭代器的实现要求:

```C++
struct random_access_iterator {
    //迭代器分类为随机访问迭代器
    using iterator_category = std::random_access_iterator_tag;
    //容器值类型
    using value_type = ?;
    //对应的指针类型
    using pointer = value_type*;
    //对应的引用类型
    using reference = value_type&;
    //迭代器差值类型
    using difference_type = std::ptrdiff_t;

    //常规的构造函数要求(Rule of Five)
    random_access_iterator();
    random_access_iterator(const random_access_iterator& rhs);
    random_access_iterator(random_access_iterator&& rhs);
    random_access_iterator& operator=(const random_access_iterator& rhs);
    random_access_iterator& operator=(random_access_iterator&& rhs);
    
    //std::swap替代实现
    void swap(random_access_iterator& iter);
	
    //比较操作实现
    bool operator==(const random_access_iterator& rhs);
    bool operator!=(const random_access_iterator& rhs);
    bool operator<(const random_access_iterator& rhs);
    bool operator<=(const random_access_iterator& rhs);
    bool operator>(const random_access_iterator& rhs);
    bool operator>=(const random_access_iterator& rhs);

    //引用、指针
    reference operator*();
    pointer operator->();
    //随机访问
    reference operator[](difference_type n);

    //递增,递减等操作
    random_access_iterator& operator++();
    random_access_iterator operator++(int);
    random_access_iterator& operator+=(difference_type n);
    random_access_iterator operator+(difference_type n);
    random_access_iterator& operator--();
    random_access_iterator operator--(int);
    random_access_iterator& operator-=(difference_type n);
    random_access_iterator operator-(difference_type n);
};

//交换实现
void swap(random_access_iterator& lhs, random_access_iterator& rhs);
```



`span_iterator`首先是基本的迭代器要求:

```C++

template <class Type>
class span_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = std::remove_cv_t<Type>;
    using difference_type = std::ptrdiff_t;
    using pointer = Type*;
    using reference = Type&;
    
};
```

内部使用三个指针来表达迭代器信息:起始位置,结束位置和当前位置:

```C++
    pointer begin_ = nullptr;
    pointer end_ = nullptr;
    pointer current_ = nullptr;
```

接着来看一下构造函数实现:

```C++
constexpr span_iterator() = default;

constexpr span_iterator(pointer begin, pointer end, pointer current)
    : begin_(begin), end_(end), current_(current)
    {}

//const版
constexpr operator span_iterator<const Type>() const noexcept
{
    return {begin_, end_, current_};
}
```


比较操作实现:

```C++
constexpr bool operator==(const span_iterator<Type>& rhs) const noexcept
{
    return current_ == rhs.current_;
}

constexpr bool operator!=(const span_iterator<Type>& rhs) const noexcept
{
    return !(*this == rhs);
}

constexpr bool operator<(const span_iterator<Type>& rhs) const noexcept
{
    Expects(begin_ == rhs.begin_ && end_ == rhs.end_);
    return current_ < rhs.current_;
}

constexpr bool operator>(const span_iterator<Type>& rhs) const noexcept
{
    return rhs < *this;
}

constexpr bool operator<=(const span_iterator<Type>& rhs) const noexcept
{
    return !(rhs < *this);
}

constexpr bool operator>=(const span_iterator<Type>& rhs) const noexcept
{
    return !(*this < rhs);
}
```

访问操作实现:

```C++
constexpr reference operator*() const noexcept
{
    Expects(begin_ && end_);
    Expects(begin_ <= current_ && current_ < end_);
    return *current_;
}

constexpr pointer operator->() const noexcept
{
    Expects(begin_ && end_);
    Expects(begin_ <= current_ && current_ < end_);
    return current_;
}
constexpr reference operator[](const difference_type n) const noexcept
{
    return *(*this + n);
}
```

递增、递减等操作实现:

```C++
constexpr span_iterator& operator++() noexcept
{
    ++current_;
    return *this;
}

constexpr span_iterator operator++(int) noexcept
{
    span_iterator ret = *this;
    ++*this;
    return ret;
}

constexpr span_iterator& operator--() noexcept
{
    --current_;
    return *this;
}

constexpr span_iterator operator--(int) noexcept
{
    span_iterator ret = *this;
    --*this;
    return ret;
}

constexpr span_iterator& operator+=(const difference_type n) noexcept
{
    current_ += n;
    return *this;
}

constexpr span_iterator operator+(const difference_type n) const noexcept
{
    span_iterator ret = *this;
    ret += n;
    return ret;
}

friend constexpr span_iterator operator+(const difference_type n,
                                         const span_iterator& rhs) noexcept
{
    return rhs + n;
}

constexpr span_iterator& operator-=(const difference_type n) noexcept
{
    current_ -= n;
    return *this;
}

constexpr span_iterator operator-(const difference_type n) const noexcept
{
    span_iterator ret = *this;
    ret -= n;
    return ret;
}

constexpr difference_type operator-(const span_iterator<Type>& rhs) const noexcept
{
    return current_ - rhs.current_;
}
```

对于`span`来讲,迭代器的构造就相对简单了:

```C++
template <class ElementType>
class span{
    using iterator = span_iterator<ElementType>;
    using reverse_iterator = std::reverse_iterator<iterator>; 
    
    constexpr iterator begin() const noexcept
    {
        const auto data = storage_.data();
        return {data, data + size(), data};
    }

    constexpr iterator end() const noexcept
    {
        const auto data = storage_.data();
        const auto endData = data + storage_.size();
        return {data, endData, endData};
    }

    constexpr reverse_iterator rbegin() const noexcept { return reverse_iterator{end()}; }
    constexpr reverse_iterator rend() const noexcept { return reverse_iterator{begin()}; }    
};
```

对于`STL`设计来讲,只要具有`begin`和`end`方法,就会被算法视为容器类,也可以用`range for`,具体的操作要看迭代器实现.

### 总结

可以看到,在C++中实现迭代器不比Python,可以说书写良好的迭代器代码量非常庞大,只能靠模板技术或者C++20的`range`来降低复杂度了.

无论如何,如果在针对'容器-like'的类型进行API设计时,能够为其提供类似于STL中容器的API,将能带来体验上的大幅提升,这也是为什么在`Qt`、`nlohmann/json`等广受好评的C++库中会提供相应的迭代器实现.






