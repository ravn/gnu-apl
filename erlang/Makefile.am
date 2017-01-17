EXTRA_DIST = erlang_APL_nif.c README apl_example.erl
distdir=$(pkglibdir)

if WANT_ERLANG

# for some reason, using libtool causes Erlang to try to load the file
# erlang_APL_nif.so.so instead of erlang_APL_nif.so. We therefore do not
# use libtool but create the make targets manually. This seems to work on
# Ubuntu but may require manual fixes on other platforms.
#
all: apl.beam apl_example.beam erlang_APL_nif.so
.erl.beam:
	erl -compile $<

erlang_APL_nif.so: erlang_APL_nif.c
	gcc -o $@ -Wall -Werror -fpic -shared -rdynamic -pthread \
 erlang_APL_nif.c -L/usr/local/lib/apl -lapl


install: erlang_APL_nif.so apl.erl apl_example.erl
	sudo $(INSTALL) $^ $(pkglibdir)/

endif
