
    
@echo off

cls
echo == Initiating system instance variables...
echo. -- Setting the variables...

:: Here you need to make some changes to suit your system.
set SOURCE=./ipal-extra
set SVN=C:\Program Files\TortoiseSVN\bin
set URL=https://svn.icub.iit.local/repos/soft/embed/embedded/arm/embody/body/abslayer/ipal-package/ipal/ipal-extra

:: Unless you want to modify the script, this is enough.

echo. %SOURCE%
echo. %SVN%
echo. %URL%
echo. ++ Done setting variables.
echo.
echo == Updating source from SVN
echo. -- Running update...
"%SVN%\TortoiseProc.exe" /command:checkout /path:"%SOURCE%" /url:"%URL%" /closeonend:1
echo. ++ Done.


echo. -- setting it RO...

attrib +R "%SOURCE%"/*.*

echo. -- Cleaning up...
set SOURCE=
set SVN=
echo. ++ Done.
    
    
    
    





