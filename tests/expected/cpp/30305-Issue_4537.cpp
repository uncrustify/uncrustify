void test_enc_api(int i, int err)
{
    if (err!=OPUS_OK || (i!=OPUS_BANDWIDTH_NARROWBAND
                         && i!=OPUS_BANDWIDTH_MEDIUMBAND&&i!=OPUS_BANDWIDTH_WIDEBAND
                         && i!=OPUS_BANDWIDTH_FULLBAND)) test_failed();
}
