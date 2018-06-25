namespace shark {
    template<class Closure>
    struct indexed_iterator {
        typedef typename boost::mpl::if_<
                boost::is_const<
                    Closure
                >,
                typename Closure::const_reference,
                typename Closure::reference
        >::type reference;
    };
}
