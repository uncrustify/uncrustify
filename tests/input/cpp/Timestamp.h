/**
 * @file Timestamp.h
 * Definition of class example::Timestamp.
 */

#ifndef __Timestamp_h_
#define __Timestamp_h_

#include <string>

namespace example {

class IStreamable;
class InStream;
class OutStream;

/**
 * Timestamp is a timestamp with nanosecond resolution.
 */
class Timestamp
  : public IStreamable
{

public:

  /**
   * Default constructor.
   */
  Timestamp();

  /**
   * Constructor.
   *
   * @param sec   The seconds
   * @param nsec  The nanoseconds
   */
  Timestamp(long sec, unsigned long nsec);

  /**
   * Destructor.
   */
  virtual ~Timestamp();

  /**
   * Adds two timestamps.
   *
   * @param rhs The other timestamp
   * @return The resulting timestamp
   */
  Timestamp operator+ (const Timestamp& rhs) const;

  /**
   * Substracts two timestamps.
   *
   * @param rhs The other timestamp
   * @return The resulting timestamp
   */
  Timestamp operator- (const Timestamp& rhs) const;

  /**
   * Compares two timestamps.
   *
   * @param rhs The other timestamp
   * @return true if timestamp is smaller than the given timestamp
   */
  bool operator< (const Timestamp& rhs) const;

  /**
   * Compares two timestamps.
   *
   * @param rhs The other timestamp
   * @return true if timestamp is greater than the given timestamp
   */
  bool operator> (const Timestamp& rhs) const;

  /**
   * Compares two timestamps.
   *
   * @param rhs The other timestamp
   * @return true if timestamp is equal to the given timestamp
   */
  bool operator== (const Timestamp& rhs) const;

  /**
   * Compares two timestamps.
   *
   * @param rhs The other timestamp
   * @return true if timestamp is not equal to the given timestamp
   */
  bool operator!= (const Timestamp& rhs) const;

  /**
   * Adds an other timestamp.
   *
   * @param rhs The other timestamp
   */
  void operator+= (const Timestamp& rhs);

  /**
   * Adds milliseconds.
   *
   * @param ms The milliseconds
   * @return The resulting timestamp
   */
  Timestamp addMilliseconds(unsigned long ms) const;

  /**
   * Adds nanoseconds.
   *
   * @param ns The nanoseconds
   * @return The resulting timestamp
   */
  Timestamp addNanoseconds(unsigned long ns) const;

  /**
   * Checks if this timestamp is zero.
   *
   * @return true if timestamp is zero
   */
  bool isZero() const;

  /**
   * Gets the milliseconds.
   * @attention Negativ timestamp return zero
   *
   * @return The milliseconds
   */
  unsigned long getMilliseconds() const;

  /**
   * Divide timestamps by two.
   *
   * @return The resulting timestamp
   */
  Timestamp divideByTwo();

  /**
   * Gets the string-representation.
   *
   * @return The string representation
   */
  std::string getString() const;

  /**
   * Gets the string-representation in milliseconds.
   *
   * @return The string representation
   */
  std::string getStringMilliseconds() const;

  /**
   * Resets the timestamp.
   */
  void reset();

  /** The seconds */
  long sec;

  /** The nanoseconds */
  unsigned long nsec;

  InStream& operator << (InStream& in);

  OutStream& operator >> (OutStream& out) const;

};
} // namespace

#endif // __Timestamp_h_
