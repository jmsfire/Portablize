Put shell33.dll and ipfilter.cfg to app folder;
Patch "SHELL32.dll" to "SHELL33.dll" in Qt5Core.dll (use hex-editor);
Patch "color: rgb(255, 255, 255);" to "color: rgb(216, 216, 216);" in qtox.exe (all entries);
Patch "toxme.io" to "0.0.0.00" in qtox.exe.

Checked version: 1.8.x

Third-party:
MinHook v1.3.3 (https://www.codeproject.com/KB/winsdk/LibMinHook.aspx)

ipfilter.cfg (ANSI, '\n' (LF) line break, with empty row at end):
All rows content blocked IP addresses ranges in 'xxx.xxx.xxx.xxx-xxx.xxx.xxx.xxx' format.

E.g.:
010.000.000.000-010.255.255.255
003.114.115.116-003.114.115.116


Means block 10/8 and 3.114.115.116. Allow all rest IP addresses.
