typedef enum {
  HAL_USART_ENABLED = 64,                   ///< Requested task impossible while
                                            ///< peripheral in question is
                                            ///< enabled
  HAL_USART_DISABLED,                       ///< Requested task impossible while
                                            ///< peripheral in question is
                                            ///< disabled
  HAL_USART_GPIO_ERROR,                     ///< GPIO tied with USART peripheral
                                            ///< returned error state
  HAL_USART_BUFFER_DEPLETED,                ///< Not enough data to be read
  HAL_USART_BUFFER_FULL                     ///< Data requested to be written
                                            ///< didn't fit into buffer
} hal_usart_errors_t;
