cmake -S . -B build -T v140 -A x64
cmake --build build --config Release
xcopy Test.py build\Release
build\Release\App.exe
