    AnyContainerBase(AnyContainerBase &&moveable) noexcept:
        SuperContainer(
            SuperContainer::Token,
            static_cast<SuperContainer &&>(moveable)
        )
    {
        auto source = moveable.container();
        source->move(container());
    }

