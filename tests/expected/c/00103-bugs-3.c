
#ifdef CRUD
#define JUNK    a
#endif

#ifdef __QNX__
/**
 * Does all that QNX trickery to get the MAC address of the interface
 *
 * @param if_name The interface name: ie "en0" or "eth0"
 * @param mac     Pointer to a 6-byte array
 * @return        SUCCESS or FAILURE
 */
static INT32 socket_get_mac_qnx(const CHAR *if_name, UINT8 *mac)
{
   CHAR  ionet_name[50];
   INT32 en_fd;

#if QNX_RELEASE >= 630
   nic_config_t nic;
   INT32        dcmd = DCMD_IO_NET_GET_CONFIG;
#else
   Nic_t        nic;
   INT32        dcmd = DCMD_IO_NET_NICINFO;
#endif
   INT32        ret_val = FAILURE;

   memset(mac, 0, 6);

   /* Build the full name */
   snprintf(ionet_name, sizeof(ionet_name), "/dev/io-net/%s", if_name);

   /* Open the device */
   en_fd = open(ionet_name, O_RDWR);
   if (en_fd >= 0)
   {
      /* Get the interface info */
      if (devctl(en_fd, dcmd, &nic, sizeof(nic), NULL) == EOK)
      {
         memcpy(mac, nic.current_address, 6);
         ret_val = SUCCESS;
      }

      close(en_fd);
   }
   return(ret_val);
}
#endif

