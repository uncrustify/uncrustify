int oldfoo(bar)
char bar;
{
   return(bar - 2);
}

int i2c_use_client(struct i2c_client *client)
{
   int ret;

   ret = i2c_inc_use_client(client);
   if (ret)
   {
      return(ret);
   }

   if ((client->flags & I2C_CLIENT_ALLOW_USE) || (a &&
                                                  something_else))
   {
      if (client->flags & I2C_CLIENT_ALLOW_MULTIPLE_USE)
      {
         client->usage_count++;
      }
      else if (client->usage_count > 0)
      {
         goto busy;
      }
      else
      {
         client->usage_count++;
      }
   }

   return(0);

 busy:
   i2c_dec_use_client(client);
   return(-EBUSY);
}

void get_name(void)
{
   a = (int)5;

   if (a)
   {
      if (b)
      {
         b--;
      }
      else
      {
         a++;
      }
   }
   for (a = 0; a < 10; a++)
   {
      if (b)
      {
         b--;
      }
      else
      {
         a++;
      }
   }
   return;
}

