if exist a.exe erase a.exe
gcc *.c ..\collections\src\*.c ..\strings\strings.c -I.\..\collections\include -I.\.. -g -Werror 
if exist a.exe a.exe