/* else is tied to if(two) */
int main() {
  if (one) {
    if (two)
      sleep (1);
    else
      sleep (2);
  }

  if (three)
      sleep(1);
  else
      if (four)
         sleep(2);
}

/* else is tied to if(one) */
int main() {
  if (one) {
    if (two)
      sleep (1);
  }
  else {
    if (three)
      sleep (2);
  }
}

/* else.1 is tied to if(two), else.2 is tied to if(one) */
int main() {
  if (one) {
    if (two)
      sleep (1);
    else {
      if (three)
	sleep (2);
    }
  }
  else
    sleep (3);
}
int main() {

 if (read_write == I2C_SMBUS_READ)
        {
         msg[1].len = I2C_SMBUS_I2C_BLOCK_MAX;
        }
       else
         {
         msg[0].len = data->block[0] + 1;
       }
}
