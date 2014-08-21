int main () {
        key_press_event.connect ((e) => {
            switch (e.keyval) {
                case Gdk.Key.@0:
                    if ((e.state & Gdk.ModifierType.CONTROL_MASK) != 0) {
                        action_zoom_default_font ();
                        return true;
                    }

                    break;
                case Gdk.Key.@1: //alt+[1-8]
                case Gdk.Key.@7:
                case Gdk.Key.@8:
                    if (((e.state & Gdk.ModifierType.MOD1_MASK) != 0) && settings.alt_changes_tab) {
                        var i = e.keyval - 49;
                        if (i > notebook.n_tabs - 1)
                            return false;

                        notebook.current = notebook.get_tab_by_index ((int) i);
                        return true;
                    }

                    break;
                default:
                    assert_not_reached () ;
            }

            return false;
        });
        
    return 0 ; 
    }
