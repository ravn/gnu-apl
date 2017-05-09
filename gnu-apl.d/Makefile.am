
dist_noinst_DATA = preferences.in keyboard1.txt parallel_thresholds

dist_DATA = preferences keyboard1.txt parallel_thresholds

distdir = ${sysconfdir}/gnu-apl.d

# update preferences when eg. ./configure --prefix is executed.
# See autoconf.pdf page 29
#
edit = sed -e 's|@prefix[@]|$(prefix)|g'

# build preferences after configure
preferences: Makefile $(srcdir)/preferences.in
	rm -f $@; srcdir=''; \
	test -f ./$@.in || srcdir=$(srcdir)/; \
	$(edit) $${srcdir}$@.in >$@

