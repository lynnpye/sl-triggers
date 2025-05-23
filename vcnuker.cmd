@echo off
echo VCPKG Scorched Earth BEGIN!
pushd %~dp0
rd /q /s build
popd
pushd d:\v
rd /q /s buildtrees downloads packages
popd
pushd c:\Users\Lynn\AppData\Local\vcpkg
rd /q /s archives registries
popd
echo All done! Have a nice day!