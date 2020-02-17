# rbdict

Example of a general purpose Red/Black tree implementation in C using
the Linux kernel rb_tree infrastructure

Written Eyal Ben-David  (bdeyal@gmail.com)
License : LGPL

Build:
make

Test:
make test

On Linux you can run:
make memtest (provided valgrind is installed)

Windows Build:
nmake -f NMakefile [test]

Test file is a word list file words.txt whic was generated
by this command on a Linux machine:

   grep -P "e.*b.*d.*" /usr/share/dict/words > words.txt


Eyal Ben-David
