#pragma once

namespace uncrustify
{

namespace limits
{

static constexpr int MAX_OPTION_NAME_LEN = 32;
static constexpr int AL_SIZE             = 8000;
static constexpr int MAX_KEYWORDS        = 300;

// uncrustify doesn't support more than one variable definition per line/ type,
// the maximum level of pointer indirection is 3 (i.e., ***p).
// TODO add some more limitations

} // namespace limits

} // namespace uncrustify
