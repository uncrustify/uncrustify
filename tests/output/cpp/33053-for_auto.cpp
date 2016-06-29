void foo()
{
    for (auto const& item : list)
        bar(item);
    for (const auto& item : list)
        bar(item);
    for (auto& item : list)
        bar(item);

    auto* var = bar();
    auto& var = bar();
    auto var = bar();
    auto const* var = bar();
    auto const& var = bar();
    auto const var = bar();
}
