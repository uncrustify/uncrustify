void wpa_smk_send_error(struct wpa_authenticator *wpa_auth,
        struct wpa_state_machine *sm, const u8 *peer,
        u16 mui, u16 error_type)
{
    u8 kde[2 + RSN_SELECTOR_LEN + ETH_ALEN +
            2 + RSN_SELECTOR_LEN + sizeof(struct rsn_error_kde)];
    u8 *pos;
    struct rsn_error_kde error;

    wpa_auth_logger(wpa_auth, sm->addr, LOGGER_DEBUG,
            "Sending SMK Error");
}
