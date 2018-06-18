class settings final
{
public:
settings();
~settings(  );
settings(const settings&);
settings & operator=(const settings&);
void set_something(const std::string& p_settings_name);
void set_another_setting(const std::string& p_settings_name);


}
