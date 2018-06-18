#ifndef NL_CLASS_H_INCLUDED
#define NL_CLASS_H_INCLUDED

#include <string>

namespace example {

	class IStreamable;
	class InStream;
	class OutStream;

/**
 * Timestamp is a timestamp with nanosecond resolution.
 */
	class Inher
		: public IStreamable {

public:
		Inher();
		virtual ~Inher();

	};

/**
 * Timestamp is a timestamp with nanosecond resolution.
 */
	class Inher2
		: public IStreamable {

public:

		Inher2();
		Inher2(long sec, unsigned long nsec);

	};

	class Simple {

public:

		Simple();
		virtual ~Simple();

	};

	class Simple2 {

public:

		Simple2();
		virtual ~Simple2();

	};

} // namespace

#endif   // NL_CLASS_H_INCLUDED
