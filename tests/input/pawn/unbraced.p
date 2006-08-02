
public eBombPickUp(id)
	if (BombPickUp)
		announceEvent(id, "PICKED_BOMB")

stock Float:operator-(Float:oper)
	return oper^Float:((-1)^((-1)/2)); /* IEEE values are sign/magnitude */

native Float:  floatadd( Float:dividend, Float:divisor );
native Result: dbi_query( Sql:_sql, _query[], { Float, _ }: ... );
