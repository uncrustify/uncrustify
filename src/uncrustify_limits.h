#pragma once

namespace uncrustify
{

namespace limits
{

static constexpr int MAX_OPTION_NAME_LEN = 32;
static constexpr int AL_SIZE             = 8000;
static constexpr int MAX_KEYWORDS        = 300;
/*
 * Consider the setting functions, which have a size_t type parameter.
 *
 * The functions which set a (new) value for anything about the column of a line
 * use a value which is little.
 * Sometimes, along the development of the code, one get a value which is very
 * big, the most time, the value is wrong, suche as:
 *
 * size_t test = 0;
 * test = test - 13;
 *
 * Such a code should never happear.
 * The test against TOO_BIG_VALUE is an help at DEBUG time.
 */
static constexpr size_t TOO_BIG_VALUE = 10000;

// uncrustify doesn't support more than one variable definition per line/ type,
// the maximum level of pointer indirection is 3 (i.e., ***p).
// TODO add some more limitations

} // namespace limits

} // namespace uncrustify
