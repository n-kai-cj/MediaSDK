@echo off

javac -cp .;jna-5.2.0.jar test/*.java
jar -cvf DecTest.jar test/*.class
