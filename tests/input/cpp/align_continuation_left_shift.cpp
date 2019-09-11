std::string foo(struct tm* local) {
    std::stringstream timestamp;
    timestamp <<
        (local->tm_year + 1900) << "." <<
        (local->tm_mon + 1) << "." <<
        local->tm_mday << "-" <<
        local->tm_hour << "." <<
        local->tm_min << "." <<
        local->tm_sec;
    return timestamp.str();
}

std::string foo2(struct tm* local) {
    std::stringstream timestamp;
    int year = local->tm_year + 1900;
    int mon = local->tm_mon + 1;
    timestamp <<
        year << "." <<
        mon << "." <<
        local->tm_mday << "-" <<
        local->tm_hour << "." <<
        local->tm_min << "." <<
        local->tm_sec;
    return timestamp.str();
}
