/*
 * Copyright (C) 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifndef WTF_StringHasher_h
#define WTF_StringHasher_h

#include <wtf/unicode/Unicode.h>

namespace WTF {

// Golden ratio - arbitrary start value to avoid mapping all 0's to all 0's
static const unsigned stringHashingStartValue = 0x9e3779b9U;

// Paul Hsieh's SuperFastHash
// http://www.azillionmonkeys.com/qed/hash.html
// char* data is interpreted as latin-encoded (zero extended to 16 bits).
class StringHasher {
public:
    inline StringHasher()
        : m_hash(stringHashingStartValue)
        , m_hasPendingCharacter(false)
        , m_pendingCharacter(0)
    {
    }

    inline void addCharacters(UChar a, UChar b)
    {
        ASSERT(!m_hasPendingCharacter);
        addCharactersToHash(a, b);
    }

    inline void addCharacter(UChar ch)
    {
        if (m_hasPendingCharacter) {
            addCharactersToHash(m_pendingCharacter, ch);
            m_hasPendingCharacter = false;
            return;
        }

        m_pendingCharacter = ch;
        m_hasPendingCharacter = true;
    }

    inline unsigned hash() const
    {
        unsigned result = m_hash;

        // Handle end case.
        if (m_hasPendingCharacter) {
            result += m_pendingCharacter;
            result ^= result << 11;
            result += result >> 17;
        }

        // Force "avalanching" of final 31 bits.
        result ^= result << 3;
        result += result >> 5;
        result ^= result << 2;
        result += result >> 15;
        result ^= result << 10;

        // First bit is used in UStringImpl for m_isIdentifier.
        result &= 0x7fffffff;

        // This avoids ever returning a hash code of 0, since that is used to
        // signal "hash not computed yet", using a value that is likely to be
        // effectively the same as 0 when the low bits are masked.
        if (!result)
            return 0x40000000;

        return result;
    }

    template<typename T, UChar Converter(T)> static inline unsigned computeHash(const T* data, unsigned length)
    {
       /* Was StringHasher hasher;
        bool rem = length & 1;
        length >>= 1;

        while (length--) {
            hasher.addCharacters(Converter(data[0]), Converter(data[1]));
            data += 2;
        }

        if (rem)
            hasher.addCharacter(Converter(*data));

        return hasher.hash();*/   
        
       StringHasher hasher;
        bool rem = length & 1;
        length >>= 1;
        while (length--) {
	    UChar b0 = 	Converter(data[0]);
	    if(!b0)
		break;	
	    UChar b1 = 	Converter(data[1]);	
	    if(b0 && b1	)
	     {
            	hasher.addCharacters(b0,b1);
            	data += 2;
	     }
	    else
		break;	 
        }
        if (rem)
        {
            UChar b0 = Converter(*data);
	    if(b0)		
	        hasher.addCharacter(b0);
        }		
        return hasher.hash();
    }

    template<typename T, UChar Converter(T)> static inline unsigned computeHash(const T* data)
    {
        StringHasher hasher;

        while (true) {
            UChar b0 = Converter(*data++);
            if (!b0)
                break;
            UChar b1 = Converter(*data++);
            if (!b1) {
                hasher.addCharacter(b0);
                break;
            }

            hasher.addCharacters(b0, b1);
        }

        return hasher.hash();
    }

    template<typename T> static inline unsigned computeHash(const T* data, unsigned length)
    {
        return computeHash<T, defaultCoverter>(data, length);
    }

    template<typename T> static inline unsigned computeHash(const T* data)
    {
        return computeHash<T, defaultCoverter>(data);
    }

    template<size_t length> static inline unsigned hashMemory(const void* data)
    {
        COMPILE_ASSERT(!(length % 4), length_must_be_a_multible_of_four);
        return computeHash<UChar>(static_cast<const UChar*>(data), length / sizeof(UChar));
    }

    static inline unsigned hashMemory(const void* data, unsigned size)
    {
        ASSERT(!(size % 2));
        return computeHash<UChar>(static_cast<const UChar*>(data), size / sizeof(UChar));
    }

private:
    static inline UChar defaultCoverter(UChar ch)
    {
        return ch;
    }

    static inline UChar defaultCoverter(char ch)
    {
        return static_cast<unsigned char>(ch);
    }

    inline void addCharactersToHash(UChar a, UChar b)
    {
        m_hash += a;
        unsigned tmp = (b << 11) ^ m_hash;
        m_hash = (m_hash << 16) ^ tmp;
        m_hash += m_hash >> 11;
    }

    unsigned m_hash;
    bool m_hasPendingCharacter;
    UChar m_pendingCharacter;
};

} // namespace WTF

using WTF::StringHasher;

#endif // WTF_StringHasher_h
