public on_damage(id)
{
#ifdef defined DEBUG && !( DEBUG_LEVEL & UNIT_TEST_DEBUG_LEVEL )

	if( g_debug_level & 4 )
	{
		set_task( 2.0, "create_fakeVotes", TASKID_DBG_FAKEVOTES );
	}
#endif
}

public on_damage(id)
{
	if( g_debug_level && 4 )
	{
		set_task( 2.0, "create_fakeVotes", TASKID_DBG_FAKEVOTES );
	}
}