
#define SetTeleType(%1,%2) set_pev( %1, pev_iuser1, %2 )
#define SetTeleMate(%1,%2) set_pev( %1, pev_iuser2, %2*7)

int main () {
#if WANT_TO_COMPILE_THIS
gtkwidget.clicked.connect( (widdget) => { message ("Clicked" ) ; }) ;
#else
gtkwidget.enabled.connect( (widdget) => {
message ("Clicked" ) ;
}) ;
#endif
return 0 ;
}
