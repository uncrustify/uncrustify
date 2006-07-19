
static void set_md_connected(CHAR *msg_data)
{
   UINT32 rd_idx = 0;
   CHAR   mobile_device_id[MOBILE_DEVICE_NAME_LEN];
   CHAR   ip_addr[IP_ADDRESS_LEN];
   CHAR   mac_addr[MAC_ADDR_LEN];
   CHAR   ap_name[AP_NAME_LEN];

   rdc_strz(msg_data, MAX_SIZE, &rd_idx, mobile_device_id, 0);
   rdc_strz(msg_data, MAX_SIZE, &rd_idx, ip_addr, 0);
   rdc_strz(msg_data, MAX_SIZE, &rd_idx, mac_addr, 0);
   rdc_strz(msg_data, MAX_SIZE, &rd_idx, ap_name, 0);

#if defined (DB_MGR_ORACLE)
   (void)db_set_md_connected(mobile_device_id, ip_addr, mac_addr, ap_name);
   LOG(LFTR, "CONNECTED Loco %s, IP Addr %s,MAC Addr %s, AP Name %s",
       mobile_device_id, ip_addr, mac_addr, ap_name);
#elif defined (DB_MGR_FILE)
   LOG(LFTR, "%s CONNECTED Loco %s, IP Addr %s,MAC Addr %s, AP Name %s",
       status_str, mobile_device_id, ip_addr, mac_addr, ap_name);
#else
#error Unknown device type must be DB_MGR_ORACLE or DB_MGR_FILE
#endif
}

