int main()
{
#if WANT_TO_COMPILE_THIS
   gtkwidget.clicked.connect((widdget) => { message("Clicked");
                             });
#else
   gtkwidget.enabled.connect((widdget) => {
      message("Clicked");
   });
#endif
   return(0);
}
