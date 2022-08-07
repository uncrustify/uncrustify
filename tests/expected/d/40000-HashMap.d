/*******************************************************************************

        @file HashMap.d

        This software is provided 'as-is', without any express or implied
        warranty. In no event will the authors be held liable for damages
        of any kind arising from the use of this software.

        Permission is hereby granted to anyone to use this software for any
        purpose, including commercial applications, and to alter it and/or
        redistribute it freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must
           not claim that you wrote the original software. If you use this
           software in a product, an acknowledgment within documentation of
           said product would be appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must
           not be misrepresented as being the original software.

        3. This notice may not be removed or altered from any distribution
           of the source.

        4. Derivative works are permitted, but they must carry this notice
           in full and credit the original source.


                        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


        Written by Doug Lea with assistance from members of JCP JSR-166
        Expert Group and released to the public domain, as explained at
        http://creativecommons.org/licenses/publicdomain

        @version        Initial version, July 2004
        @author         Doug Lea; ported/modified by Kris

*******************************************************************************/

module mango.cache.HashMap;

/******************************************************************************

******************************************************************************/

extern (C)
{
int memcmp(char *, char *, uint);
}


/**
 * A hash table supporting full concurrency of retrievals and
 * adjustable expected concurrency for updates. This class obeys the
 * same functional specification as {@link java.util.Hashtable}, and
 * includes versions of methods corresponding to each method of
 * <tt>Hashtable</tt>. However, even though all operations are
 * thread-safe, retrieval operations do <em>not</em> entail locking,
 * and there is <em>not</em> any support for locking the entire table
 * in a way that prevents all access.  This class is fully
 * interoperable with <tt>Hashtable</tt> in programs that rely on its
 * thread safety but not on its synchronization details.
 *
 * <p> Retrieval operations (including <tt>get</tt>) generally do not
 * block, so may overlap with update operations (including
 * <tt>put</tt> and <tt>remove</tt>). Retrievals reflect the results
 * of the most recently <em>completed</em> update operations holding
 * upon their onset.  For aggregate operations such as <tt>putAll</tt>
 * and <tt>clear</tt>, concurrent retrievals may reflect insertion or
 * removal of only some entries.  Similarly, Iterators and
 * Enumerations return elements reflecting the state of the hash table
 * at some point at or since the creation of the iterator/enumeration.
 * They do <em>not</em> throw
 * {@link ConcurrentModificationException}.  However, iterators are
 * designed to be used by only one thread at a time.
 *
 * <p> The allowed concurrency among update operations is guided by
 * the optional <tt>concurrencyLevel</tt> constructor argument
 * (default 16), which is used as a hint for internal sizing.  The
 * table is internally partitioned to try to permit the indicated
 * number of concurrent updates without contention. Because placement
 * in hash tables is essentially random, the actual concurrency will
 * vary.  Ideally, you should choose a value to accommodate as many
 * threads as will ever concurrently modify the table. Using a
 * significantly higher value than you need can waste space and time,
 * and a significantly lower value can lead to thread contention. But
 * overestimates and underestimates within an order of magnitude do
 * not usually have much noticeable impact. A value of one is
 * appropriate when it is known that only one thread will modify and
 * all others will only read. Also, resizing this or any other kind of
 * hash table is a relatively slow operation, so, when possible, it is
 * a good idea to provide estimates of expected table sizes in
 * constructors.
 *
 * <p>This class and its views and iterators implement all of the
 * <em>optional</em> methods of the {@link Map} and {@link Iterator}
 * interfaces.
 *
 * <p> Like {@link java.util.Hashtable} but unlike {@link
 * java.util.HashMap}, this class does NOT allow <tt>null</tt> to be
 * used as a key or value.
 *
 * <p>This class is a member of the
 * <a href="{@docRoot}/../guide/collections/index.html">
 * Java Collections Framework</a>.
 *
 * @since 1.5
 * @author Doug Lea
 * @param <K> the type of keys maintained by this map
 * @param <V> the type of mapped values
 */

class HashMap
{
    alias void[] K;
    alias Object V;
    alias jhash hash;           // jhash, fnv, or walter

    /*
     * The basic strategy is to subdivide the table among Segments,
     * each of which itself is a concurrently readable hash table.
     */

    /* ---------------- Constants -------------- */

    /**
     * The default initial number of table slots for this table.
     * Used when not otherwise specified in constructor.
     */
    private const uint DEFAULT_INITIAL_CAPACITY = 16;

    /**
     * The maximum capacity, used if a higher value is implicitly
     * specified by either of the constructors with arguments.  MUST
     * be a power of two <= 1<<30 to ensure that entries are indexible
     * using ints.
     */
    private const uint MAXIMUM_CAPACITY = 1 << 30;

    /**
     * The default load factor for this table.  Used when not
     * otherwise specified in constructor.
     */
    private const float DEFAULT_LOAD_FACTOR = 0.75f;

    /**
     * The default number of concurrency control segments.
     **/
    private const uint DEFAULT_SEGMENTS = 16;

    /**
     * The maximum number of segments to allow; used to bound
     * constructor arguments.
     */
    private const uint MAX_SEGMENTS = 1 << 16; // slightly conservative


    /* ---------------- Fields -------------- */

    /**
     * Mask value for indexing into segments. The upper bits of a
     * key's hash code are used to choose the segment.
     **/
    private final int segmentMask;

    /**
     * Shift value for indexing within segments.
     **/
    private final int segmentShift;

    /**
     * The segments, each of which is a specialized hash table
     */
    private final Segment[] segments;


    /* ---------------- Small Utilities -------------- */

    /**
     * Returns a hash code for non-null Object x.
     * Uses the same hash code spreader as most other java.util hash tables.
     * @param x the object serving as a key
     * @return the hash code
     */
    private static final uint walter(K x)
    {
        uint h = typeid(char[]).getHash(&x);

        h += ~(h << 9);
        h ^= (h >>> 14);
        h += (h << 4);
        h ^= (h >>> 10);
        return h;
    }

    /**
     * Returns a hash code for non-null Object x.
     * uses the FNV hash function
     * @param x the object serving as a key
     * @return the hash code
     */
    private static final uint fnv(K x)
    {
        uint hash = 2_166_136_261;

        foreach (ubyte c; cast(ubyte[])x)
        {
            hash ^= c;
            hash *= 16_777_619;
        }
        return hash;
    }



    /**
     * hash() -- hash a variable-length key into a 32-bit value
     *   k     : the key (the unaligned variable-length array of bytes)
     *   len   : the length of the key, counting by bytes
     *   level : can be any 4-byte value
     * Returns a 32-bit value.  Every bit of the key affects every bit of
     * the return value.  Every 1-bit and 2-bit delta achieves avalanche.
     * About 36+6len instructions.
     *
     * The best hash table sizes are powers of 2.  There is no need to do
     * mod a prime (mod is sooo slow!).  If you need less than 32 bits,
     * use a bitmask.  For example, if you need only 10 bits, do
     *   h = (h & hashmask(10));
     * In which case, the hash table should have hashsize(10) elements.
     *
     * If you are hashing n strings (ub1 **)k, do it like this:
     *   for (i=0, h=0; i<n; ++i) h = hash( k[i], len[i], h);
     *
     * By Bob Jenkins, 1996.  bob_jenkins@burtleburtle.net.  You may use this
     * code any way you wish, private, educational, or commercial.  It's free.
     *
     * See http://burlteburtle.net/bob/hash/evahash.html
     * Use for hash table lookup, or anything where one collision in 2^32 is
     * acceptable. Do NOT use for cryptographic purposes.
     */

    static final uint jhash(K x)
    {
        ubyte *k;
        uint  a,
              b,
              c,
              len;

        len = x.length;
        k   = cast(ubyte *)x;
        a   = b = 0x9e3779b9;

        // the previous hash value
        c = 0;

        // handle most of the key
        while (len >= 12)
        {
            a += *cast(uint *)(k + 0);
            b += *cast(uint *)(k + 4);
            c += *cast(uint *)(k + 8);

            a -= b; a -= c; a ^= (c >> 13);
            b -= c; b -= a; b ^= (a << 8);
            c -= a; c -= b; c ^= (b >> 13);
            a -= b; a -= c; a ^= (c >> 12);
            b -= c; b -= a; b ^= (a << 16);
            c -= a; c -= b; c ^= (b >> 5);
            a -= b; a -= c; a ^= (c >> 3);
            b -= c; b -= a; b ^= (a << 10);
            c -= a; c -= b; c ^= (b >> 15);
            k += 12; len -= 12;
        }

        // handle the last 11 bytes
        c += x.length;
        switch (len)
        {
        case 11: c += (cast(uint)k[10] << 24);

        case 10: c += (cast(uint)k[9] << 16);

        case 9: c += (cast(uint)k[8] << 8);

        case 8: b += (cast(uint)k[7] << 24);

        case 7: b += (cast(uint)k[6] << 16);

        case 6: b += (cast(uint)k[5] << 8);

        case 5: b += k[4];

        case 4: a += (cast(uint)k[3] << 24);

        case 3: a += (cast(uint)k[2] << 16);

        case 2: a += (cast(uint)k[1] << 8);

        case 1: a += k[0];

        default:
        }

        a -= b; a -= c; a ^= (c >> 13);
        b -= c; b -= a; b ^= (a << 8);
        c -= a; c -= b; c ^= (b >> 13);
        a -= b; a -= c; a ^= (c >> 12);
        b -= c; b -= a; b ^= (a << 16);
        c -= a; c -= b; c ^= (b >> 5);
        a -= b; a -= c; a ^= (c >> 3);
        b -= c; b -= a; b ^= (a << 10);
        c -= a; c -= b; c ^= (b >> 15);

        return c;
    }


    /**
     * Returns the segment that should be used for key with given hash
     * @param hash the hash code for the key
     * @return the segment
     */
    private final Segment segmentFor(uint hash)
    {
        return segments[(hash >>> segmentShift) & segmentMask];
    }

    /* ---------------- Inner Classes -------------- */

    /**
     * ConcurrentHashMap list entry. Note that this is never exported
     * out as a user-visible Map.Entry.
     *
     * Because the value field is volatile, not final, it is legal wrt
     * the Java Memory Model for an unsynchronized reader to see null
     * instead of initial value when read via a data race.  Although a
     * reordering leading to this is not likely to ever actually
     * occur, the Segment.readValueUnderLock method is used as a
     * backup in case a null (pre-initialized) value is ever seen in
     * an unsynchronized access method.
     */
    private static class HashEntry
    {
        final K         key;
        final uint      hash;
        final V         value;
        final HashEntry next;

        this(K key, uint hash, HashEntry next, V value)
        {
            this.key   = key;
            this.hash  = hash;
            this.next  = next;
            this.value = value;
        }
    }

    /**
     * Segments are specialized versions of hash tables.  This
     * subclasses from ReentrantLock opportunistically, just to
     * simplify some locking and avoid separate construction.
     **/
    static class Segment
    {
        /*
         * Segments maintain a table of entry lists that are ALWAYS
         * kept in a consistent state, so can be read without locking.
         * Next fields of nodes are immutable (final).  All list
         * additions are performed at the front of each bin. This
         * makes it easy to check changes, and also fast to traverse.
         * When nodes would otherwise be changed, new nodes are
         * created to replace them. This works well for hash tables
         * since the bin lists tend to be short. (The average length
         * is less than two for the default load factor threshold.)
         *
         * Read operations can thus proceed without locking, but rely
         * on selected uses of volatiles to ensure that completed
         * write operations performed by other threads are
         * noticed. For most purposes, the "count" field, tracking the
         * number of elements, serves as that volatile variable
         * ensuring visibility.  This is convenient because this field
         * needs to be read in many read operations anyway:
         *
         *   - All (unsynchronized) read operations must first read the
         *     "count" field, and should not look at table entries if
         *     it is 0.
         *
         *   - All (synchronized) write operations should write to
         *     the "count" field after structurally changing any bin.
         *     The operations must not take any action that could even
         *     momentarily cause a concurrent read operation to see
         *     inconsistent data. This is made easier by the nature of
         *     the read operations in Map. For example, no operation
         *     can reveal that the table has grown but the threshold
         *     has not yet been updated, so there are no atomicity
         *     requirements for this with respect to reads.
         *
         * As a guide, all critical volatile reads and writes to the
         * count field are marked in code comments.
         */

        /**
         * The number of elements in this segment's region.
         **/
        int count;

        /**
         * The table is rehashed when its size exceeds this threshold.
         * (The value of this field is always (int)(capacity *
         * loadFactor).)
         */
        int threshold;

        /**
         * The per-segment table. Declared as a raw type, casted
         * to HashEntry<K,V> on each use.
         */
        HashEntry[] table;

        /**
         * The load factor for the hash table.  Even though this value
         * is same for all segments, it is replicated to avoid needing
         * links to outer object.
         * @serial
         */
        final float loadFactor;

        this(int initialCapacity, float lf)
        {
            loadFactor = lf;
            setTable(new HashEntry[initialCapacity]);
        }

        /**
         * Set table to new HashEntry array.
         * Call only while holding lock or in constructor.
         **/
        private final void setTable(HashEntry[] newTable)
        {
            threshold = cast(int)(newTable.length * loadFactor);
            volatile table = newTable;
        }

        /**
         * Return properly casted first entry of bin for given hash
         */
        private final HashEntry getFirst(uint hash)
        {
            HashEntry[] tab;

            volatile tab = table;
            return tab[hash & (tab.length - 1)];
        }

        /**
         * Return true if the two keys match
         */
        private static final bool matchKey(K a, K b)
        {
            if (a.length == b.length)
                return cast(bool)(memcmp(cast(char *)a, cast(char *)b, a.length) == 0);

            return false;
        }

        /* Specialized implementations of map methods */

        final V get(K key, uint hash)
        {
            int c;

            // read-volatile
            volatile c = count;
            if (c)
            {
                HashEntry e = getFirst(hash);

                while (e)
                {
                    if (hash == e.hash && matchKey(key, e.key))
                    {
                        V v;

                        volatile v = e.value;
                        if (v)
                            return v;

                        synchronized (this)
                        return e.value;
                    }
                    e = e.next;
                }
            }
            return null;
        }


        final bool containsKey(K key, uint hash)
        {
            int c;

            // read-volatile
            volatile c = count;
            if (c)
            {
                HashEntry e = getFirst(hash);

                while (e)
                {
                    if (e.hash == hash && matchKey(key, e.key))
                        return true;

                    e = e.next;
                }
            }
            return false;
        }



        final synchronized V replace(K key, uint hash, V newValue)
        {
            HashEntry e = getFirst(hash);

            while (e && (e.hash != hash || !matchKey(key, e.key)))
                e = e.next;

            V oldValue = null;

            if (e)
                volatile
                {
                    oldValue = e.value;
                    e.value  = newValue;
                }
            return oldValue;
        }


        final synchronized V put(K key, uint hash, V value, bool onlyIfAbsent)
        {
            int c;

            volatile c = count;
            if (c++ > threshold)
                rehash();

            HashEntry[] tab;

            volatile tab = table;
            uint      index = hash & (tab.length - 1);
            HashEntry first = tab[index];
            HashEntry e     = first;

            while (e && (e.hash != hash || !matchKey(key, e.key)))
                e = e.next;

            V oldValue;

            if (e)
            {
                volatile oldValue = e.value;
                if (!onlyIfAbsent)
                    volatile e.value = value;
            }
            else
            {
                oldValue   = null;
                tab[index] = new HashEntry(key, hash, first, value);

                // write-volatile
                volatile count = c;
            }
            return oldValue;
        }


        private final void rehash()
        {
            HashEntry[] oldTable;

            volatile oldTable = table;
            int oldCapacity = oldTable.length;

            if (oldCapacity >= MAXIMUM_CAPACITY)
                return;

            /*
             * Reclassify nodes in each list to new Map.  Because we are
             * using power-of-two expansion, the elements from each bin
             * must either stay at same index, or move with a power of two
             * offset. We eliminate unnecessary node creation by catching
             * cases where old nodes can be reused because their next
             * fields won't change. Statistically, at the default
             * threshold, only about one-sixth of them need cloning when
             * a table doubles. The nodes they replace will be garbage
             * collectable as soon as they are no longer referenced by any
             * reader thread that may be in the midst of traversing table
             * right now.
             */

            HashEntry[] newTable = new HashEntry[oldCapacity << 1];

            threshold = cast(int)(newTable.length * loadFactor);
            int sizeMask = newTable.length - 1;

            for (int i = 0; i < oldCapacity; ++i)
            {
                // We need to guarantee that any existing reads of old Map can
                //  proceed. So we cannot yet null out each bin.
                HashEntry e = oldTable[i];

                if (e)
                {
                    HashEntry next = e.next;
                    uint      idx  = e.hash & sizeMask;

                    //  Single node on list
                    if (next is null)
                        newTable[idx] = e;
                    else
                    {
                        // Reuse trailing consecutive sequence at same slot
                        HashEntry lastRun = e;
                        int       lastIdx = idx;

                        for (HashEntry last = next; last; last = last.next)
                        {
                            uint k = last.hash & sizeMask;

                            if (k != lastIdx)
                            {
                                lastIdx = k;
                                lastRun = last;
                            }
                        }
                        newTable[lastIdx] = lastRun;

                        // Clone all remaining nodes
                        for (HashEntry p = e; p !is lastRun; p = p.next)
                        {
                            uint      k = p.hash & sizeMask;
                            HashEntry n = newTable[k];

                            newTable[k] = new HashEntry(p.key, p.hash, n, p.value);
                        }
                    }
                }
            }
            volatile table = newTable;
        }

        /**
         * Remove; match on key only if value null, else match both.
         */
        final synchronized V remove(K key, uint hash, V value)
        {
            int         c;
            HashEntry[] tab;

            volatile c = count - 1;
            volatile tab = table;

            uint      index = hash & (tab.length - 1);
            HashEntry first = tab[index];
            HashEntry e     = first;

            while (e && (e.hash != hash || !matchKey(key, e.key)))
                e = e.next;

            V oldValue = null;

            if (e)
            {
                V v;

                volatile v = e.value;
                if (value is null || value == v)
                {
                    oldValue = v;

                    // All entries following removed node can stay
                    // in list, but all preceding ones need to be
                    // cloned.
                    HashEntry newFirst = e.next;

                    for (HashEntry p = first; p !is e; p = p.next)
                        newFirst = new HashEntry(p.key, p.hash, newFirst, p.value);
                    tab[index] = newFirst;

                    // write-volatile
                    volatile count = c;
                }
            }
            return oldValue;
        }


        final synchronized void clear()
        {
            if (count)
            {
                HashEntry[] tab;

                volatile tab = table;

                for (int i = 0; i < tab.length; i++)
                    tab[i] = null;

                // write-volatile
                volatile count = 0;
            }
        }
    }



    /* ---------------- Public operations -------------- */

    /**
     * Creates a new, empty map with the specified initial
     * capacity and the specified load factor.
     *
     * @param initialCapacity the initial capacity. The implementation
     * performs internal sizing to accommodate this many elements.
     * @param loadFactor  the load factor threshold, used to control resizing.
     * @param concurrencyLevel the estimated number of concurrently
     * updating threads. The implementation performs internal sizing
     * to try to accommodate this many threads.
     * @throws IllegalArgumentException if the initial capacity is
     * negative or the load factor or concurrencyLevel are
     * nonpositive.
     */
    public this(uint initialCapacity, float loadFactor, uint concurrencyLevel)
    {
        assert(loadFactor > 0);

        if (concurrencyLevel > MAX_SEGMENTS)
            concurrencyLevel = MAX_SEGMENTS;

        // Find power-of-two sizes best matching arguments
        int sshift = 0;
        int ssize  = 1;

        while (ssize < concurrencyLevel)
        {
            ++sshift;
            ssize <<= 1;
        }

        segmentShift  = 32 - sshift;
        segmentMask   = ssize - 1;
        this.segments = new Segment[ssize];

        if (initialCapacity > MAXIMUM_CAPACITY)
            initialCapacity = MAXIMUM_CAPACITY;

        int c = initialCapacity / ssize;

        if (c * ssize < initialCapacity)
            ++c;

        int cap = 1;

        while (cap < c)
            cap <<= 1;

        for (int i = 0; i < this.segments.length; ++i)
            this.segments[i] = new Segment(cap, loadFactor);
    }

    /**
     * Creates a new, empty map with the specified initial
     * capacity,  and with default load factor and concurrencyLevel.
     *
     * @param initialCapacity The implementation performs internal
     * sizing to accommodate this many elements.
     * @throws IllegalArgumentException if the initial capacity of
     * elements is negative.
     */
    public this(uint initialCapacity)
    {
        this(initialCapacity, DEFAULT_LOAD_FACTOR, DEFAULT_SEGMENTS);
    }

    /**
     * Creates a new, empty map with a default initial capacity,
     * load factor, and concurrencyLevel.
     */
    public this()
    {
        this(DEFAULT_INITIAL_CAPACITY, DEFAULT_LOAD_FACTOR, DEFAULT_SEGMENTS);
    }

    /**
     * Returns the value to which the specified key is mapped in this table.
     *
     * @param   key   a key in the table.
     * @return  the value to which the key is mapped in this table;
     *          <tt>null</tt> if the key is not mapped to any value in
     *          this table.
     * @throws  NullPointerException  if the key is
     *               <tt>null</tt>.
     */
    public V get(K key)
    {
        uint hash = hash(key); // throws NullPointerException if key null

        return segmentFor(hash).get(key, hash);
    }

    /**
     * Tests if the specified object is a key in this table.
     *
     * @param   key   possible key.
     * @return  <tt>true</tt> if and only if the specified object
     *          is a key in this table, as determined by the
     *          <tt>equals</tt> method; <tt>false</tt> otherwise.
     * @throws  NullPointerException  if the key is
     *               <tt>null</tt>.
     */
    public bool containsKey(K key)
    {
        uint hash = hash(key); // throws NullPointerException if key null

        return segmentFor(hash).containsKey(key, hash);
    }

    /**
     * Maps the specified <tt>key</tt> to the specified
     * <tt>value</tt> in this table. Neither the key nor the
     * value can be <tt>null</tt>.
     *
     * <p> The value can be retrieved by calling the <tt>get</tt> method
     * with a key that is equal to the original key.
     *
     * @param      key     the table key.
     * @param      value   the value.
     * @return     the previous value of the specified key in this table,
     *             or <tt>null</tt> if it did not have one.
     * @throws  NullPointerException  if the key or value is
     *               <tt>null</tt>.
     */
    public V put(K key, V value)
    {
        assert(value);

        uint hash = hash(key);

        return segmentFor(hash).put(key, hash, value, false);
    }

    /**
     * If the specified key is not already associated
     * with a value, associate it with the given value.
     * This is equivalent to
     * <pre>
     *   if (!map.containsKey(key))
     *      return map.put(key, value);
     *   else
     *      return map.get(key);
     * </pre>
     * Except that the action is performed atomically.
     * @param key key with which the specified value is to be associated.
     * @param value value to be associated with the specified key.
     * @return previous value associated with specified key, or <tt>null</tt>
     *         if there was no mapping for key.
     * @throws NullPointerException if the specified key or value is
     *            <tt>null</tt>.
     */
    public V putIfAbsent(K key, V value)
    {
        assert(value);

        uint hash = hash(key);

        return segmentFor(hash).put(key, hash, value, true);
    }


    /**
     * Removes the key (and its corresponding value) from this
     * table. This method does nothing if the key is not in the table.
     *
     * @param   key   the key that needs to be removed.
     * @return  the value to which the key had been mapped in this table,
     *          or <tt>null</tt> if the key did not have a mapping.
     * @throws  NullPointerException  if the key is
     *               <tt>null</tt>.
     */
    public V remove(K key)
    {
        uint hash = hash(key);

        return segmentFor(hash).remove(key, hash, null);
    }

    /**
     * Remove entry for key only if currently mapped to given value.
     * Acts as
     * <pre>
     *  if (map.get(key).equals(value)) {
     *     map.remove(key);
     *     return true;
     * } else return false;
     * </pre>
     * except that the action is performed atomically.
     * @param key key with which the specified value is associated.
     * @param value value associated with the specified key.
     * @return true if the value was removed
     * @throws NullPointerException if the specified key is
     *            <tt>null</tt>.
     */
    public bool remove(K key, V value)
    {
        uint hash = hash(key);

        return cast(bool)(segmentFor(hash).remove(key, hash, value) !is null);
    }


    /**
     * Replace entry for key only if currently mapped to some value.
     * Acts as
     * <pre>
     *  if ((map.containsKey(key)) {
     *     return map.put(key, value);
     * } else return null;
     * </pre>
     * except that the action is performed atomically.
     * @param key key with which the specified value is associated.
     * @param value value to be associated with the specified key.
     * @return previous value associated with specified key, or <tt>null</tt>
     *         if there was no mapping for key.
     * @throws NullPointerException if the specified key or value is
     *            <tt>null</tt>.
     */
    public V replace(K key, V value)
    {
        assert(value);

        uint hash = hash(key);

        return segmentFor(hash).replace(key, hash, value);
    }


    /**
     * Removes all mappings from this map.
     */
    public void clear()
    {
        for (int i = 0; i < segments.length; ++i)
            segments[i].clear();
    }


    /**
     * Returns an enumeration of the keys in this table.
     *
     * @return  an enumeration of the keys in this table.
     * @see     #keySet
     */
    public KeyIterator keys()
    {
        return new KeyIterator(this);
    }

    /**
     * Returns an enumeration of the values in this table.
     *
     * @return  an enumeration of the values in this table.
     * @see     #values
     */
    public ValueIterator elements()
    {
        return new ValueIterator(this);
    }

    /**********************************************************************

            Iterate over all keys in hashmap

    **********************************************************************/

    int opApply(int delegate(inout char[]) dg)
    {
        int         result   = 0;
        KeyIterator iterator = keys();

        while (iterator.hasNext)
        {
            char[] ca = cast(char[])iterator.next;

            if ((result = dg(ca)) != 0)
                break;
        }
        return result;
    }

    /**********************************************************************

            Iterate over all keys in hashmap

    **********************************************************************/

    int opApply(int delegate(inout char[], inout Object) dg)
    {
        int         result   = 0;
        KeyIterator iterator = keys();

        while (iterator.hasNext)
        {
            HashEntry he = iterator.nextElement;
            char[]    ca = cast(char[])he.key;

            if ((result = dg(ca, he.value)) != 0)
                break;
        }
        return result;
    }


    /* ---------------- Iterator Support -------------- */

    abstract static class HashIterator
    {
        int         nextSegmentIndex;
        int         nextTableIndex;
        HashEntry[] currentTable;
        HashEntry   nextEntry;
        HashEntry   lastReturned;
        HashMap     map;

        this(HashMap map)
        {
            this.map         = map;
            nextSegmentIndex = map.segments.length - 1;
            nextTableIndex   = -1;
            advance();
        }

        final void advance()
        {
            if (nextEntry !is null && (nextEntry = nextEntry.next) !is null)
                return;

            while (nextTableIndex >= 0)
            {
                if ((nextEntry = currentTable[nextTableIndex--]) !is null)
                    return;
            }

            while (nextSegmentIndex >= 0)
            {
                Segment seg = map.segments[nextSegmentIndex--];

                volatile if (seg.count)
                    {
                        currentTable = seg.table;
                        for (int j = currentTable.length - 1; j >= 0; --j)
                        {
                            if ((nextEntry = currentTable[j]) !is null)
                            {
                                nextTableIndex = j - 1;
                                return;
                            }
                        }
                    }
            }
        }

        public bool hasNext()
        {
            return cast(bool)(nextEntry !is null);
        }

        HashEntry nextElement()
        {
            if (nextEntry is null)
                throw new Exception("no such element in HashMap");

            lastReturned = nextEntry;
            advance();
            return lastReturned;
        }
    }

    static class KeyIterator : HashIterator
    {
        this(HashMap map) {
            super(map);
        }
        public K next()
        {
            return super.nextElement().key;
        }
    }

    static class ValueIterator : HashIterator
    {
        this(HashMap map) {
            super(map);
        }
        public V next()
        {
            volatile return super.nextElement().value;
        }
    }
}
