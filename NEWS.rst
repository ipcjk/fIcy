fIcy 1.0.22: 2018-01-xx
-----------------------

* Use getaddrinfo to resolve hostnames.
* Resolve service names as well as port numbers everywhere.


fIcy 1.0.21: 2017-08-10
-----------------------

* fIcy can now append to output files directly using ``-A``.
* Error messages about missing or incompatible flags are now clearer.


fIcy 1.0.20: 2017-06-23
-----------------------

* fPls accepts more (broken) EXTM3U files.
* fIcy output file (``-o``) is no longer overwritten if ``-n`` is requested
  without metadata-based splitting.
* fIcy output file is now correctly not interrupted if titles are requested
  without flags affecting filename generation (``-t``, ``-E`` or ``-m``).
* fIcy now builds fine by default on systems using gcc 4.7.
* Support for building with ``pmake`` under IRIX has been removed.


fIcy 1.0.19: 2015-01-11
-----------------------

* fPls will now correctly retry loading remote playlists on connection
  timeout/failures according to the requested limit (3 by default).
* fPls/fIcy ``-T`` (maximum time) flag now takes connection/retry delays
  into account, ensuring the requested time is never exceeded.
* Some examples in the documentation have been clarified.


fIcy 1.0.18: 2011-03-29
-----------------------

* Fixed build with recent C++ compilers/libs.


fIcy 1.0.17: 2009-11-19
-----------------------

* Fixed "redirection limit" error message.
* Fixed build with newer GLIBC/GCC versions.
* Fixed -f/-F under linux.


fIcy 1.0.16: 2007-01-03
-----------------------

* Better (full) timeout support on both fIcy/fPls.
* -e removed on fIcy (use -E1 to get the same effect instead).
* fPls now also forwards -il flags automatically.
* fPls no longer waits after last failure before exiting.
* Now honouring also HTTP 301 redirects.
* GNU sed limitation removed: the rewrite coprocessor is now customizable
  (defaulting to "sed" like before).
* Work around OSX's HFS+ charset requirements.


fIcy 1.0.15: 2005-11-09
-----------------------

* Relaxed redirection checks.
* Exit with success after recording time is elapsed.
* Some code cleanup.


fIcy 1.0.14: 2005-09-01
-----------------------

* Basic HTTP authentication support was implemented (-a).
* Idle network timeout support was implemented (-i).
* Enumeration starting number can now be specified/autodetected (-E).
* fResync was unable to sync small files in mmap mode.


fIcy 1.0.13: 2005-05-11
-----------------------

* Relaxed the playlist parser (fixes behaviour on "fancy" playlists).
* -xXI can now be used with -o alone (dump matching title/s on the specified
  file without rewriting).
* Added fPls daemon support.


fIcy 1.0.12: 2005-04-18
-----------------------

* HTTP redirection handling was implemented.
* Support for abort-on-clobber (-i) was dropped.


fIcy 1.0.11: 2005-02-11
-----------------------

* Added a maximum playing time option (-M).
* Exit values are more strict now.
* Dropped "ohg" for `rst <http://docutils.sourceforge.net/>`_.


fIcy 1.0.10: 2004-12-06
-----------------------

* Added a title filtering/rewriting mechanism (-fF).
* Include/exclude expressions can be loaded from a file now (-I).
* Fixed other endianess problems affecting the URL parser.


fIcy 1.0.9: 2004-12-03
----------------------

* Malicious .pls files could contain an option that would be passed along to
  fIcy (harmless, but fixed).
* Fixed argument parsing in fPls under GNU/glibc systems.
* Fixed documentation for fPls (-W was really -T).
* fResync now correctly refuses to operate on read-only files.
* Fixed fResync endianess problems.


fIcy 1.0.8: 2004-11-15
----------------------

* Bug in fResync truncation code.
* New utility: fPls: playlist handler.


fIcy 1.0.7: 2004-11-06
----------------------

* Using -x did not reset the file for non-matching titles.
* New -X flag: exclude matching titles (complements -x).
* Multiple -x and -X can be combined on the command line.


fIcy 1.0.6: 2004-11-02
----------------------

* OpenBSD compilation fixes (thanks to Tobias Franke "deimoz").
* SHOUTcast > 1.9 compatibility [aka: bug in SHOUTcast] fixes.
* Reduced HTTP's request fragmentation.


fIcy 1.0.5: 2004-09-06
----------------------

* WARNING: -i has changed semantics!!!
* -p has changed semantics. Look into README's example section.
* New -x flag: save only matching titles.
* New -q flag: save file ordering.
* New utility fResync: cleanup badly cut MPEG files.


fIcy 1.0.4: 2004-05-03
----------------------

* ohg now included into the distribution.
* Better filename sanitization.
* Terminal output sanitization.
* Better error reporting. No more "unexpected ICY reply".


fIcy 1.0.3: 2004-04-09
----------------------

* Support for removing partial dumps.
* URL parsing on the command line.
* New -r flag to remove partial dumps.


fIcy 1.0.2: 2004-03-15
----------------------

* SIGPIPE handler
* public release!


fIcy 1.0.1: Oct 2003
--------------------

* Now works on linux.


fIcy 1.0.0: earlyer in 2003
---------------------------

* Now in "C"(r)


fIcy 0.0.0: late 2002
---------------------

* Original source::

    #!/bin/sh
    netcat "$1" "$2" << EOF | sed -e "1,9d"
    GET $3 HTTP/1.0
    Host: $1:$2

    EOF
