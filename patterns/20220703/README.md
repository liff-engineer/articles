> [Embedding Python in a C++ project with Visual Studio](https://devblogs.microsoft.com/python/embedding-python-in-a-cpp-project-with-visual-studio/)

示例演示了如何为`C++`应用程序提供`Python`插件,工程分为以下内容:

- `Actor`:`C++`库,提供了接口定义和工厂类,供插件实现并注册;
- `PyActor`:基于`pybind11`的`Actor`库`Python`接口模块;
- `PyPlugin`:基于`pybind11`,能够加载`Python`模块的`C++`插件;
- `App`:基于`C++`的应用程序,加载`PyPlugin`等插件,从`Actor`工厂创建接口并运行;
- `Test.py`:基于`Python`的模块,供`PyPlugin`加载.

在`build.bat`中执行了以下操作,可以分步骤执行:

```bash
cmake -S . -B build -T v140 -A x64
cmake --build build --config Release
xcopy Test.py build\Release
build\Release\App.exe
```

1. 生成`.sln`;
2. 构建;
3. 拷贝`Test.py`到输出目录,以保证`MyPlugin.dll`正确加载;
4. 运行`App.exe`.

注意,上述指令要求开发者本地安装有`64bit`的`Python`版本.