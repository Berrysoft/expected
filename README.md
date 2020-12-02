# Expected
An unusual `std::expected` implementation by C++ coroutine.

It is inspired by [tl::expected](https://github.com/TartanLlama/expected), and `Result<T, E>` in Rust.
Coroutine is used to make it clear.

## Example
``` cpp
expected::task<image, fail_reason> get_cute_cat(image img)
{
    // Use `eerror` here to indicate it is an error;
    // It is OK to be without it, if `image` and `fail_reason` is not the same type.
    if (!img) co_return expected::eerror, fail_reason::invalid_image;
    auto cropped = co_await crop_to_cat(img);
    auto with_tie = co_await add_bow_tie(cropped);
    auto with_sparkles = co_await make_eyes_sparkle(with_tie);
    co_return add_rainbow(make_smaller(with_sparkles));
}

std::optional<image> get_cute_cat_sync(image img)
{
    // `task` could be waited synchronously.
    auto t = get_cute_cat(img);
    auto result = t.run();
    // `value` is an `optional`.
    return result.value();
}
```
