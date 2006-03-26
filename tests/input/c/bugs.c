typedef struct {
   int a;
   char b;
} foo_t;

s32 i2c_smbus_read_i2c_block_data(struct i2c_client *client, u8 command, u8 *values)
{
	union i2c_smbus_data data;
	int i;
	if (i2c_smbus_xfer(client->adapter,client->addr,client->flags,
	                      I2C_SMBUS_READ,command,
	                      I2C_SMBUS_I2C_BLOCK_DATA,&data))
		return -1;
	else {
		for (i=1;i<=data.block[0];i ++)
			values[i-1] = data.block[i];
		return data.block[0];
	}
}

void foo(void)
{
   adap->nr=  id &  MAX_ID_MASK;

   list_for_each(item,&drivers) {
      driver=list_entry(item, struct i2c_driver, list);
      if (driver->detach_adapter)
         if ((res = driver->detach_adapter(adap)))
         {
            dev_err(&adap->dev, "detach_adapter failed "
                    "for driver [%s]\n", driver->name);
            goto out_unlock;
         }
   }
}
