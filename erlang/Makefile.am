EXTRA_DIST = erlang_APL_nif.c README apl_example.erl
distdir=$(pkglibdir)

if WANT_ERLANG

# for some reason, using libtool causes Erlang to try to load the file
# erlang_APL_nif.so.so instead of erlang_APL_nif.so. We therefore do not
# use libtool but create the make targets manually. This seems to work on
# Ubuntu but may require manual fixes on other platforms.
#
FILES = apl.beam apl_example.beam erlang_APL_nif.so

CLEANFILES = ${FILES}

all: apl.beam apl_example.beam erlang_APL_nif.so
.erl.beam:
	erlc $<

erlang_APL_nif.so: erlang_APL_nif.c
	gcc -o $@ -Wall $(WERROR) -fpic -shared -rdynamic -pthread \
 erlang_APL_nif.c -L/usr/local/lib/apl -lapl


install: erlang_APL_nif.so apl.erl apl_example.erl
	sudo $(INSTALL) $^ $(pkglibdir)/

else

.PHONY:	all
all:
	echo "nothing to be done because GNU APL was NOT configured --with-erlang"

endif

