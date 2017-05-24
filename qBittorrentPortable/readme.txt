x64 version:
Put shell33.dll and ipfilter.cfg to app folder;
Patch "SHELL32.dll" to "SHELL33.dll" in Qt5Core.dll (use hex-editor).
x32 version:
Put powrproz.dll and ipfilter.cfg to app folder;
Patch "POWRPROF.dll" to "POWRPROZ.dll" in qbittorrent.exe (use hex-editor).

Checked version: 3.3.12

Third-party (x64):
MinHook v1.3.3 (https://www.codeproject.com/KB/winsdk/LibMinHook.aspx)


ipfilter.cfg (ANSI, '\n' (LF) line break, with empty row at end):
All rows content blocked IP addresses ranges in 'xxx.xxx.xxx.xxx-xxx.xxx.xxx.xxx' format.

E.g.:
010.000.000.000-010.255.255.255
003.114.115.116-003.114.115.116


Means block 10/8 and 3.114.115.116. Allow all rest IP addresses.
