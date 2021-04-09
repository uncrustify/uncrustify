obj.AddObject(Object::UniqueName(), 10, [this] {
    holder.Access([this](const auto &info) {
    if (IsGood(info)) {
        Add(info);
    }
});
});

obj.AddObject(
    Object::UniqueName(),
    10,
    [this] {
    holder.Access([this](const auto &info) {
    if (IsGood(info)) {
        Add(info);
    }
});
}
);

{
    obj.AddObject(Object::UniqueName(), 10, [this] {
        holder.Access([this](const auto &info) {
        if (IsGood(info)) {
            Add(info);
        }
    });
    });

    obj.AddObject(
        Object::UniqueName(),
        10,
        [this] {
        holder.Access([this](const auto &info) {
        if (IsGood(info)) {
            Add(info);
        }
    });
    }
    );
}
