@echo off
echo Formatting source files...

forfiles /s /m *.h /c "cmd /c clang-format -i -style=file @path"
forfiles /s /m *.cpp /c "cmd /c clang-format -i -style=file @path"

forfiles /s /m *.h /c "cmd /c clang-format -i -style=file @path"
forfiles /s /m *.cpp /c "cmd /c clang-format -i -style=file @path"
