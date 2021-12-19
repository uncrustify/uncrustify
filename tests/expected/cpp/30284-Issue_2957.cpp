bus_type i2c_bus_type =
{
	.name    = "i2c",
	.match   = i2c_device_match,
	.suspend = i2c_bus_suspend,
	.resume  = i2c_bus_resume,
};
