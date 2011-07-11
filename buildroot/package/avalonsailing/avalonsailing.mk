#############################################################
#
# avalon
#
#############################################################
AVALONSAILING_VERSION = $(shell date --iso)
AVALONSAILING_SOURCE = %SOURCE%
AVALONSAILING_SITE = http://www/~lbedford/buildroot/

define AVALONSAILING_BUILD_CMDS
	$(MAKE) AR="$(TARGET_AR)" CC="$(TARGET_CC)" \
		CXX="$(TARGET_CXX)" -C $(@D)
endef

define AVALONSAILING_INSTALL_TARGET_CMDS
	$(MAKE) DESTDIR=$(TARGET_DIR) install -C $(@D)
	$(MAKE) DESTDIR=$(TARGET_DIR) installconf -C $(@D)
	$(MAKE) DESTDIR=$(TARGET_DIR) installinit -C $(@D)
	test -f $(TARGET_DIR)/etc/udev/rules.d/10-avalonsailing.rules || \
		$(INSTALL) -D -m 755 package/avalonsailing/avalonsailing.udev \
		$(TARGET_DIR)/etc/udev/rules.d/10-avalonsailing.rules
endef

AVALONSAILING_INSTALL_STAGING = NO
AVALONSAILING_INSTALL_TARGET = YES

$(eval $(call GENTARGETS,package,avalonsailing))
