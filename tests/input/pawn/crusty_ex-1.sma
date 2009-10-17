public hudtext16(textblock[],colr,colg,colb,posx,posy,screen,time,id)
{
	new y
	if(contain(textblock,"^n") == -1) { // if there is no linebreak in the text, we can just show it as it is
		set_hudmessage(colr, colg, colb, float(posx)/1000.0, float(posy)/1000.0, 0, 6.0, float(time), 0.2, 0.2, screen)
		show_hudmessage(id,textblock)
	}
	else { // more than one line
		new out[128],rowcounter=0,tmp[512],textremain=true;y=screen

		new i = contain(textblock,"^n")
		
		do
		{
		}
		while(textremain > 0);
		
		copy(out,i,textblock) // we need to get the first line of text before the loop
		
		do
		{
		}
		while(textremain)
	}
	return screen-y // we will return how many screens of text we printed
}
