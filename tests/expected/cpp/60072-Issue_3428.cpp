void main()
{
    if(aaaa)
    {
        it = std::find_if(
            bbbb.begin(),
            bbbb.end(),
            [&cccc](const auto& dddd)
            {
                return (eeee.ffff == iiii && !jjjj.kkkk);
            }
        );
    }
}

namespace ns1 {

    void one()
    {
        if(aaaa)
        {
            it = std::find_if(
                bbbb.begin(),
                bbbb.end(),
                [&cccc](const auto& dddd)
                {
                    return (eeee.ffff == iiii && !jjjj.kkkk);
                }
            );
        }
    }

    namespace ns2 {
        namespace ns3 {
            const auto lamb = []() -> int
            {
                return 42;
            };

            void two()
            {
                if(aaaa)
                {
                    it = std::find_if(
                        bbbb.begin(),
                        bbbb.end(),
                        [&cccc](const auto& dddd)
                        {
                            return (eeee.ffff == iiii && !jjjj.kkkk);
                        }
                    );
                }
            }

        }
    }
}
