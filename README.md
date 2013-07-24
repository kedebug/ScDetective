ScDetective
==============================================================

#### A kernel level Anti-Rootkit tool which runs on the windows platform.   

ScDetective is my very first project, and it's currently in a very alpha state.   
It was finished in my third year in college, at that time I was addicted to the    
windows driver programming and accumulated lot of debug skills.    

Thanks to the great open source spirit, without previous work I couldn't do all     
this alone. Thanks to the bbs.pediy.com forum, it gave me so much happiness and    
unforgetable memories in my college life.

If you have any suggestion or questions, please feel free to get in touch via sunweiqq@gmail.com.

- GUI :         VS2008 + MFC
- Driver :        VS2005 + ddkwizard
- DDK Version：   7600.16385.1
- Debug ：        Windbg + VirtualKD + VMware
- Platform :      XPSP3 + WIN7
- Finished :          2010.12

#### There are about 6 modules in the ScDetective_Driver content:  
- Detect and restore the SSDT and shadow SSDT.
- Detect and static judging the active processes.
- Detect and static judging the drivers.
- HookEngine module and part of the work was reversed from CNNIC driver.
- Self-protect module(some DKOM skills).
- A demo file filter driver depended on sfilter library(NOT finished).

#### kedebug   
Department of Computer Science and Engineering,   
Shanghai Jiao Tong University   
E-mail: sunweiqq@gmail.com   
