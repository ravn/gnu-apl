
# 1. targets defined in Makefile.incl can be build before ./configure
#
.PHONY: help develop develop_lib parallel parallel1
help develop develop_lib parallel parallel1:
	make -f Makefile.incl $@

# 2. all other targets shall run ./configure first
#
%:
	rm -f Makefile
	./configure
	make $@

