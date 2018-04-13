extern struct device device_list[];
extern struct device device_list_end[];

static int
device_probe(struct device *dev)
{
	int err;
}

/* ===================== */
struct scpi_mem {
	struct scpi_msg tx_msg; /**< The reply to be sent to a client. */
	struct scpi_msg rx_msg; /**< The request received from a client. */
};

struct scpi_buffer {
	struct scpi_mem mem;    /**< Memory for the request/reply messages. */
	uint8_t         client; /**< Client that should receive the reply. */
	bool            busy;   /**< Flag telling if this buffer is in use. */
};

static void
scpi_receive_message(struct device *dev __unused, uint8_t client, uint32_t msg)
{
	struct scpi_buffer *buffer;
	struct scpi_msg    *rx_msg = &SCPI_MEM_AREA(client).rx_msg;

	assert(dev == scpi_msgbox);
}
