public on_damage(id)
{
    new attacker = get_user_attacker(id)

#if defined DAMAGE_RECIEVED
    // id should be connected if this message is sent, but lets check anyway
    if ( is_user_connected(id) && is_user_connected(attacker) )
    if (get_user_flags(attacker) & ADMIN_LEVEL_H)
    {
        new damage = read_data(2)

        set_hudmessage(255, 0, 0, 0.45, 0.50, 2, 0.1, 4.0, 0.1, 0.1, -1)
        ShowSyncHudMsg(id, g_MsgSync2, "%i^n", damage)
#else
    if ( is_user_connected(attacker) && (get_user_flags(attacker) & ADMIN_LEVEL_H) )
    {
        new damage = read_data(2)
#endif
        set_hudmessage(0, 100, 200, -1.0, 0.55, 2, 0.1, 4.0, 0.02, 0.02, -1)
        ShowSyncHudMsg(attacker, g_MsgSync, "%i^n", damage)
    }
}