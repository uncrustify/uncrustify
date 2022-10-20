MOCK_METHOD(bool, isThat, (const std::string& name), (override));
MOCK_METHOD(bool, isThat, (std::string& name), (override));
MOCK_METHOD(bool, isThat, std::string& name, (override));
MOCK_METHOD(bool, isThat, const std::string& name, (override));
MyFunction(const std::string& some_value);
