Put shfolder.dll to app folder.
Checked version: 7.30.x.x

hosts.cfg (ANSI, '\n' (LF) line break, without empty row at end):
First row: 'W' - whitelist, 'B' - blacklist;
Rest rows:
'+' - allow, '-' - disallow connections to host;
If host starts with '+.' or '-.' - allow/disallow connections to all subdomains.

E.g.:
W
+examle.com
-.example.com

Means allow example.com, but not subdomains. Allow all rest domains.



ipfilter.cfg (ANSI, '\n' (LF) line break, with empty row at end):
All rows content blocked IP addresses ranges in 'xxx.xxx.xxx.xxx-xxx.xxx.xxx.xxx' format.

E.g.:
010.000.000.000-010.255.255.255
003.114.115.116-003.114.115.116


Means block 10/8 and 3.114.115.116. Allow all rest IP addresses.
