I2C to SPI converter

This is a simple I2C to SPI bridge. It's implemented in an
ATTiny85. The ATTiny85 acts as an I2C slave, and relays
the data out a SPI port. 

The implemenation is unidirection, e.g. supports only writes
to I2C.

The implementation only expects the device address, and then
two bytes of data. Once the two bytes of data are received,
they are instantly shoved out of SPI. During the SPI phase
the ATiny85 is unresponsive to I2C. 

All ports are implemented using bitbang.

Requires the ATTiny85 to be fused to run in 8MHz RC osc mode.

Few I2C error cases are handled. The device will successfully
ignore all commands to devices that aren't it's address, but
once you address the device, you  *must* send two bytes to it.
If you're short or you're too long, the behavior isn't defined.

This code is targeted to AtmelStudio. It's not for Arduino.
