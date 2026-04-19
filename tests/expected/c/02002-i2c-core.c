/* i2c-core.c - a device driver for the iic-bus interface                    */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 1995-99 Simon G. Vogl
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.                */
/* ------------------------------------------------------------------------- */

/* With some changes from Kyösti Mälkki <kmalkki@cc.hut.fi>.
 * All SMBus-related things are written by Frodo Looijaard <frodol@dds.nl>
 * SMBus 2.0 support by Mark Studebaker <mdsxyz123@yahoo.com>                */

#include <linux/module.h>
#include <linux/kernel.h>


static LIST_HEAD(adapters);
static LIST_HEAD(drivers);

static int i2c_bus_suspend(struct device *dev, pm_message_t state)
{
   int rc = 0;

   if (dev->driver && dev->driver->suspend)
   {
      rc = dev->driver->suspend(dev, state, 0);
   }
   return(rc);
}

struct bus_type i2c_bus_type =
{
   .name    = "i2c",
   .match   = i2c_device_match,
   .suspend = i2c_bus_suspend,
   .resume  = i2c_bus_resume,
};

void i2c_adapter_dev_release(struct device *dev)
{
   struct i2c_adapter *adap = dev_to_i2c_adapter(dev);

   complete(&adap->dev_released);
}

static ssize_t show_adapter_name(struct device *dev, struct device_attribute *attr, char *buf)
{
   struct i2c_adapter *adap = dev_to_i2c_adapter(dev);

   return(sprintf(buf, "%s\n", adap->name));
}
static DEVICE_ATTR(name, S_IRUGO, show_adapter_name, NULL);


/*
 * We can't use the DEVICE_ATTR() macro here as we want the same filename for a
 * different type of a device.  So beware if the DEVICE_ATTR() macro ever
 * changes, this definition will also have to change.
 */
static struct device_attribute dev_attr_client_name =
{
   .attr = { .name = "name", .mode = S_IRUGO, .owner = THIS_MODULE },
   .show = &show_client_name,
};


int i2c_del_adapter(struct i2c_adapter *adap)
{
   struct list_head   *item, *_n;
   struct i2c_adapter *adap_from_list;
   int                res = 0;

   down(&core_lists);

   /* First make sure that this adapter was ever added */
   list_for_each_entry(adap_from_list, &adapters, list)
   {
      if (adap_from_list == adap)
      {
         break;
      }
   }
   if (adap_from_list != adap)
   {
      pr_debug("i2c-core: attempting to delete unregistered "
               "adapter [%s]\n", adap->name);
      res = -EINVAL;
      goto out_unlock;
   }

   /* detach any active clients. This must be done first, because
    * it can fail; in which case we give up. */
   list_for_each_safe(item, _n, &adap->clients)
   {
      client = list_entry(item, struct i2c_client, list);

      /* detaching devices is unconditional of the set notify
       * flag, as _all_ clients that reside on the adapter
       */
      if ((res = client->driver->detach_client(client)))
      {
         dev_err(&adap->dev, "detach_client failed for client "
                 "[%s] at address 0x%02x\n", client->name,
                 client->addr);
         goto out_unlock;
      }
   }

   /* clean up the sysfs representation */
   init_completion(&adap->dev_released);
   list_del(&adap->list);

   /* wait for sysfs to drop all references */
   wait_for_completion(&adap->dev_released);

   /* free dynamically allocated bus id */
   idr_remove(&i2c_adapter_idr, adap->nr);

   dev_dbg(&adap->dev, "adapter [%s] unregistered\n", adap->name);

 out_unlock:
   up(&core_lists);
   return(res);
} /* i2c_del_adapter */


/* The SMBus parts */

#define POLY    (0x1070U << 3)
static u8
crc8(u16 data)
{
   int i;

   for (i = 0; i < 8; i++)
   {
      if (data & 0x8000)
      {
         data = data ^ POLY;
      }
      data = data << 1;
   }
   return((u8)(data >> 8));
}

s32 i2c_smbus_xfer(struct i2c_adapter *adapter, u16 addr, unsigned short flags,
                   char read_write, u8 command, int size,
                   union i2c_smbus_data *data)
{
   s32 res;
   int swpec   = 0;
   u8  partial = 0;

   flags &= I2C_M_TEN | I2C_CLIENT_PEC;
   if ((flags & I2C_CLIENT_PEC) &&
       !(i2c_check_functionality(adapter, I2C_FUNC_SMBUS_HWPEC_CALC)))
   {
      swpec = 1;
      if (read_write == I2C_SMBUS_READ &&
          size == I2C_SMBUS_BLOCK_DATA)
      {
         size = I2C_SMBUS_BLOCK_DATA_PEC;
      }
      else if (size == I2C_SMBUS_BLOCK_PROC_CALL)
      {
         i2c_smbus_add_pec(addr, command,
                           I2C_SMBUS_BLOCK_DATA, data);
         partial = data->block[data->block[0] + 1];
         size    = I2C_SMBUS_BLOCK_PROC_CALL_PEC;
      }
      else if (read_write == I2C_SMBUS_WRITE &&
               size != I2C_SMBUS_QUICK &&
               size != I2C_SMBUS_I2C_BLOCK_DATA)
      {
         size = i2c_smbus_add_pec(addr, command, size, data);
      }
   }

   if (res >= 0 && swpec &&
       size != I2C_SMBUS_QUICK && size != I2C_SMBUS_I2C_BLOCK_DATA &&
       (read_write == I2C_SMBUS_READ || size == I2C_SMBUS_PROC_CALL_PEC ||
        size == I2C_SMBUS_BLOCK_PROC_CALL_PEC))
   {
      if (i2c_smbus_check_pec(addr, command, size, partial, data))
      {
         return(-1);
      }
   }
   return(res);
} /* i2c_smbus_xfer */


/* Next four are needed by i2c-isa */
EXPORT_SYMBOL_GPL(i2c_adapter_dev_release);
EXPORT_SYMBOL_GPL(i2c_adapter_driver);

MODULE_AUTHOR("Simon G. Vogl <simon@tk.uni-linz.ac.at>");
MODULE_DESCRIPTION("I2C-Bus main module");
MODULE_LICENSE("GPL");
