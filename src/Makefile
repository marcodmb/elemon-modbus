elemon-modbus: elemon-modbus.o
	$(CC) $(LDFLAGS) -L/usr/src/openwrt/backfire/staging_dir/target-mips_uClibc-0.9.30.1/usr/lib -L/usr/src/openwrt/backfire/staging_dir/target-mips_uClibc-0.9.30.1/usr/lib/mysql -lmysqlclient elemon-modbus.o -o elemon-modbus
elemon-modbus.o: elemon-modbus.c
	$(CC) $(CFLAGS) -I/usr/src/openwrt/backfire/staging_dir/target-mips_uClibc-0.9.30.1/usr/include/mysql -c elemon-modbus.c
clean: rm *.o elemon-modbus
