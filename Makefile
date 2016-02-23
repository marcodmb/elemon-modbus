include $(TOPDIR)/rules.mk

PKG_NAME:=elemon-modbus
PKG_RELEASE:=1
PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/elemon-modbus
	SECTION:=utils
	CATEGORY:=Utilities
	TITLE:=Electric Monitoring - GE Multimeter
endef

define Package/elemon-modbus/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/elemon-modbus $(1)/bin/
endef

$(eval $(call BuildPackage,elemon-modbus))
