SettingsDelta::SettingsDelta(
      const LastEffectiveContextData& lastEffCtxData)
   : Member2(lastEffCtxData.member2())
   , Member3(lastEffCtxData.member3().c_str())
   , Functor([this](const int& num) { Callback(num); })
   , Member4(lastEffCtxData.member4().c_str())
   , Member5(lastEffCtxData.member5())
{
}
