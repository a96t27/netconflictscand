include $(TOPDIR)/rules.mk

PKG_NAME:=netconflictscand
PKG_RELEASE:=1
PKG_VERSION:=1.0.0

include $(INCLUDE_DIR)/package.mk

define Package/netconflictscand
	CATEGORY := Net
	TITLE:=Netconflictscand
endef

define Package/netconflictscand/description
	Daemon that scans for conflicts in network
endef

define Package/netconflictscand/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bin/netconflictscand $(1)/usr/bin
	$(INSTALL_BIN) ./files/netconflictscand.init $(1)/etc/init.d/netconflictscand
endef

$(eval $(call BuildPackage,netconflictscand))
