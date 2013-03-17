#include <limits>
#include <cassert>

#include <boost/optional.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/type_traits/make_unsigned.hpp>

template<typename T>
class bit_buffer
{
public:
	bit_buffer() : buf(3), front_idx(0), back_idx(0)
	{
		buf.push_back();
	}
	template<typename U>
	boost::optional<U> pop(std::size_t bits)
	{
		assert(bits <= static_cast<std::size_t>(std::numeric_limits<U>::digits));
		if(bits > size()) return boost::optional<U>();
		typedef typename boost::make_unsigned<U>::type uu_type;
		uu_type u = 0;
		
		while(front_bits() < bits) {
			u |= (mask<uu_type>(front_bits()) & buf.front()) << (bits - front_bits());
			bits -= front_bits();
			buf.pop_front();
			front_idx = 0;
		}
		u |= mask<uu_type>(bits) & (buf.front() >> (front_bits() - bits));
		front_idx += bits;
		buf.front() &= ~mask<buf_type>(front_bits());
		return u;
	}
	template<typename U>
	U pop_force(std::size_t bits)
	{
		return *pop<U>(bits > size() ? size() : bits);
	}
	template<typename U>
	void push(U u, std::size_t bits)
	{
		assert(bits <= static_cast<std::size_t>(std::numeric_limits<U>::digits));
		typedef typename boost::make_unsigned<U>::type uu_type;
		uu_type u_ = u;
		while(bits + back_idx >= buf_type_bits) {
			buf.back() |= (mask<buf_type>(buf_type_bits - back_idx) & (u_ >> (bits + back_idx - buf_type_bits)));
			u_ = u_ & mask<uu_type>(bits + back_idx - buf_type_bits);
			bits -= buf_type_bits - back_idx;
			if(buf.full()) {
				buf.set_capacity(buf.capacity() + 1);
			}
			buf.push_back(); back_idx = 0;
		}
		buf.back() |= (mask<buf_type>(bits) & u_) << (buf_type_bits - back_idx - bits); back_idx += bits;
	}
	std::size_t capacity() const
	{
		return buf_type_bits * buf.capacity();
	}
	std::size_t reserve() const
	{
		return capacity() - size();
	}
	std::size_t size() const
	{
		return buf_type_bits * (buf.size() - 1) - front_idx + back_idx;
	}
private:
	// TODO: prohibit signed integer
	template<typename U>
	U mask(int bits) { return std::numeric_limits<U>::digits == bits ? -1 : (static_cast<U>(1) << bits) - 1; }
	typedef typename boost::make_unsigned<T>::type buf_type;
	static const std::size_t buf_type_bits = std::numeric_limits<buf_type>::digits;
	boost::circular_buffer<buf_type> buf;
//  empty: size() == 1
//  0 1 2 3 4 5 6 7
//   x x x x x x x x
//    ^front_idx
//    ^back_idx
//  nearly empty: size() == 1
//  0 1 2 3 4 5 6 7
//   x x x x x x x x
//    ^front_idx
//        ^back_idx
//  nearly full: full() == true
//    NOTE: even though full() == true, always &front() != &back()
//  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//   x x x x x x x x x x x x x x x x
//                    ^front_idx
//                ^back_idx
//  normal: otherwise
//  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//   x x x x x x x x x x x x x x x x
//    ^front_idx
//                          ^back_idx

	std::size_t front_idx, back_idx;
	std::size_t front_avail_bits() { return buf.size() == 1 ? back_idx - front_idx : buf_type_bits - front_idx; }
	std::size_t front_bits() { return buf_type_bits - front_idx; }
};

#if 0
template<typename Ch, int max_bits>
class bit_input_wrapper
{
public:
	bit_input_wrapper(Ch *&begin, Ch * const &end) : begin(begin), end(end) {}
	void reset(Ch *&begin, Ch * const &end)
	{
		begin = begin; // Rebind
		end = end;     // Rebind
	}
	template<typename T>
	boost::optional<T> read(int bits)
	{
		assert(bits <= max_bits);
	}
	template<typename T>
	T read_force(int bits)
	{
	}
	bool check(int bits = max_bits)
	{
	}
private:
	boost::optional<Ch*&> begin;
	boost::optional<Ch* const &> end;
};
#endif
