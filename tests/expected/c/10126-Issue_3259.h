int (*found_proc) (struct i2c_adapter *, int, int);

static int i2c_probe_address(struct i2c_adapter *adapter, int addr, int kind,
                             int (*found_proc1) (struct i2c_adapter *, int, int),
                             int (*found_proc2) (struct i2c_adapter *, int, int));
