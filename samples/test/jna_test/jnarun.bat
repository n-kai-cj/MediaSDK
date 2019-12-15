@echo off

set LIBS="./;./jna-5.2.0.jar;./DecTest.jar"

echo "---- javac"
javac -cp %LIBS% *.java

echo "---- java exe"
java -cp %LIBS% JnaTest

echo "---- finish"

pause
