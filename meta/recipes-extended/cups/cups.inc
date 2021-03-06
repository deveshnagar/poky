SUMMARY = "An Internet printing system for Unix"
SECTION = "console/utils"
LICENSE = "GPLv2 & LGPLv2"
DEPENDS = "gnutls libpng jpeg dbus dbus-glib zlib libusb"
PROVIDES = "cups14"

SRC_URI = "http://www.cups.org/software/${PV}/${BP}-source.tar.bz2 \
           file://use_echo_only_in_init.patch \
           file://0001-don-t-try-to-run-generated-binaries.patch \
           file://cups_serverbin.patch \
           file://cups-no-gcrypt.patch \
           file://cups-str4402.patch \
	  "

LEAD_SONAME = "libcupsdriver.so"

inherit autotools-brokensep binconfig

PACKAGECONFIG ??= "${@bb.utils.contains('DISTRO_FEATURES', 'zeroconf', 'avahi', '', d)} \
                   ${@bb.utils.contains('DISTRO_FEATURES', 'pam', 'pam', '', d)}"
PACKAGECONFIG[avahi] = "--enable-avahi,--disable-avahi,avahi"
PACKAGECONFIG[acl] = "--enable-acl,--disable-acl,acl"
PACKAGECONFIG[pam] = "--enable-pam, --disable-pam, libpam"

EXTRA_OECONF = " \
               --enable-gnutls \
               --enable-dbus \
               --enable-browsing \
               --disable-openssl \
               --disable-gssapi \
               --enable-debug \
               --disable-relro \
               --enable-libusb \
               --without-php \
               --without-perl \
               --without-python \
               --without-java \
               "


do_configure() {
	gnu-configize
	libtoolize --force
	DSOFLAGS="${LDFLAGS}" SERVERBIN="${libdir}/cups" oe_runconf
}

do_compile () {
	sed -i s:STRIP:NOSTRIP: Makedefs
	sed -i s:serial:: backend/Makefile

	echo "all:"    >  man/Makefile
	echo "libs:" >> man/Makefile
	echo "install:" >> man/Makefile
	echo "install-data:" >> man/Makefile
	echo "install-exec:" >> man/Makefile
	echo "install-headers:" >> man/Makefile
	echo "install-libs:" >> man/Makefile

	oe_runmake "SSLLIBS=-lgnutls -L${STAGING_LIBDIR}" \
		   "LIBPNG=-lpng -lm -L${STAGING_LIBDIR}" \
		   "LIBJPEG=-ljpeg -L${STAGING_LIBDIR}" \
		   "LIBZ=-lz -L${STAGING_LIBDIR}" \
		   "-I."
}

fakeroot do_install () {
	oe_runmake "DSTROOT=${D}" install

	# Remove /var/run from package as cupsd will populate it on startup
	rm -fr ${D}/${localstatedir}/run
	rmdir ${D}/${libdir}/${BPN}/driver
}

python do_package_append() {
    import subprocess
    # Change permissions back the way they were, they probably had a reason...
    workdir = d.getVar('WORKDIR', True)
    subprocess.call('chmod 0511 %s/install/cups/var/run/cups/certs' % workdir, shell=True)
}

PACKAGES =+ "${PN}-lib ${PN}-libimage"

FILES_${PN} += "${libdir}/cups/backend \
		${libdir}/cups/cgi-bin \
		${libdir}/cups/filter \
		${libdir}/cups/monitor \
		${libdir}/cups/notifier \
		${libdir}/cups/daemon \
	       "

FILES_${PN}-lib = "${libdir}/libcups.so.*"

FILES_${PN}-libimage = "${libdir}/libcupsimage.so.*"

FILES_${PN}-dbg += "${libdir}/cups/backend/.debug \
                    ${libdir}/cups/cgi-bin/.debug \
                    ${libdir}/cups/filter/.debug \
                    ${libdir}/cups/monitor/.debug \
                    ${libdir}/cups/notifier/.debug \
                    ${libdir}/cups/daemon/.debug \
                   "

#package the html for the webgui inside the main packages (~1MB uncompressed)

FILES_${PN} += "${datadir}/doc/cups/images \
                ${datadir}/doc/cups/*html \
                ${datadir}/doc/cups/*.css \
                ${datadir}/icons/ \
               "
CONFFILES_${PN} += "${sysconfdir}/cups/cupsd.conf"

SYSROOT_PREPROCESS_FUNCS += "cups_sysroot_preprocess"
cups_sysroot_preprocess () {
	sed -i ${SYSROOT_DESTDIR}${bindir_crossscripts}/cups-config -e 's:cups_datadir=.*:cups_datadir=${datadir}/cups:' -e 's:cups_serverbin=.*:cups_serverbin=${libdir}/cups:'
}
