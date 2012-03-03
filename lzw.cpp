#include <map>
#include <vector>
#include <queue>

#include <boost/iostreams/concepts.hpp>
#include <boost/iostreams/put.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/filter/test.hpp>

#include <boost/intrusive/set.hpp>
#include <boost/array.hpp>
#include <boost/optional.hpp>

#include <boost/concept/requires.hpp>
#include <boost/concept_check.hpp>

struct lzw_trie_node;
struct lzw_trie_node_comparator
{
	bool operator()(const lzw_trie_node &v1, const lzw_trie_node &v2) const;
};
struct lzw_trie_node : boost::intrusive::set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link> >
{
	unsigned char value;
	boost::intrusive::set<lzw_trie_node,
	  boost::intrusive::compare<lzw_trie_node_comparator>,
	  boost::intrusive::base_hook<boost::intrusive::set_base_hook<boost::intrusive::link_mode<boost::intrusive::normal_link> > > > next;
	template<typename Cloner, typename Disposer>
	void clone_from(const lzw_trie_node& from, Cloner c, Disposer d)
	{
		value = from.value;
		next.clone_from(from.next, c, d);
	}
	lzw_trie_node() : value(0), next() {}
private:
	lzw_trie_node(const lzw_trie_node&);
	lzw_trie_node& operator=(const lzw_trie_node&);
};
bool lzw_trie_node_comparator::operator()(const lzw_trie_node &v1, const lzw_trie_node &v2) const
{
	return v1.value < v2.value;
}

struct lzw_trie
{
	static unsigned short bits(unsigned short us)
	{
		return ((us >> 12) & 3) + 9;
	}
	static unsigned short entry(unsigned short us)
	{
		return us & 0xFFF;
	}
	static unsigned short make_value(unsigned short num, unsigned short value)
	{
		unsigned short bits;
		if(num < 512) bits = 0;
		else if(num < 1024) bits = 1;
		else if(num < 2048) bits = 2;
		else if(num < 4096) bits = 3;
		else throw 1; // FIX: throw proper exception
		return (bits << 12)| (value & 0xFFF);
	}
	lzw_trie(bool early_change) : num(258), early_change(early_change)
	{
		for(unsigned short uc = 0; uc < 256; ++uc) {
			block[uc].value = uc;
			root.next.insert(block[uc]);
		}
	}
	lzw_trie(const lzw_trie& r) : num(r.num), last_found(&root), early_change(r.early_change), eod(false)
	{
		for(std::size_t i = 0; i < r.block.size(); ++i) {
			block[i].value = r.block[i].value;
			block[i].next.clone_from(r.block[i].next, fake_cloner(&r.block[0], &block[0]), null_disposer());
		}
		root.clone_from(r.root, fake_cloner(&r.block[0], &block[0]), null_disposer());
	}
	template<typename Iterator>
	BOOST_CONCEPT_REQUIRES(
		((boost::Convertible<typename boost::ForwardIterator<Iterator>::value_type, unsigned char>)),
		(boost::optional<unsigned short>))
	step(Iterator &first, Iterator last)
	{
		if(num == (early_change ? 4095 : 4096)) {
			clear();
			return make_value(4095, 256); // clear-table
		}
//std::cerr << "searching" << std::endl;
		while(first != last) {
			if(last_found->next.count(*first, comparator())) {
				last_found = &*last_found->next.find(*first, comparator());
//std::cerr << "found: " << static_cast<int>(*first) << " as " << last_found << std::endl;
				++first;
			} else {
//std::cerr << "not found: " << static_cast<int>(*first) << ", registering as " << num << std::endl;
				block[num].next.clear();
				block[num].value = *first;
				last_found->next.insert(block[num]);
				unsigned short result = last_found - &block[0];
				last_found = &root;
				++num;
				return make_value(num - (early_change ? 0 : 1), result);
			}
		}
		return boost::optional<unsigned short>();
	}
	boost::optional<unsigned short> flush()
	{
		if(last_found != &root) {
			unsigned short result = last_found - &block[0];
			last_found = &root;
			return make_value(num - (early_change ? 0 : 1), result);
		} else if(!eod) {
			eod = true;
			return make_value(num - (early_change ? 0 : 1), 257); // EOD
		}
		return boost::optional<unsigned short>();
	}
	void clear()
	{
		root.next.clear();
		for(unsigned short uc = 0; uc < 256; ++uc) {
			block[uc].next.clear();
			root.next.insert(block[uc]);
		}
		num = 258;
		last_found = &root;
	}
	void reset()
	{
		eod = false;
		clear();
	}
	int entries()
	{
		return num;
	}
	// TODO: unify with codes in make_value()
	unsigned short num_bits()
	{
		if(num < 512) return 9;
		else if(num < 1024) return 10;
		else if(num < 2048) return 11;
		else if(num < 4096) return 12;
		else throw 1; // FIX: throw proper exception
	}
private:
	struct comparator
	{
		bool operator()(const lzw_trie_node& v1, unsigned char v2) const { return v1.value < v2; }
		bool operator()(unsigned char v1, const lzw_trie_node& v2) const { return v1 < v2.value; }
	};
	struct fake_cloner
	{
		const lzw_trie_node* const from_head;
		lzw_trie_node* const to_head;
		fake_cloner(const lzw_trie_node* from_head, lzw_trie_node* to_head) : from_head(from_head), to_head(to_head) {}
		lzw_trie_node* operator()(const lzw_trie_node& from) const
		{
			return to_head + (&from - from_head);
		}
	};
	struct null_disposer
	{
		void operator()(lzw_trie_node*) const {}
	};
	unsigned short num;
	boost::array<lzw_trie_node, 4096> block;
// can be boost::intrusive::set, but cutting corners
	lzw_trie_node root, *last_found;
	bool early_change, eod;
};

struct lzw_decompressor : boost::iostreams::multichar_dual_use_filter
{
	template<typename Device>
	std::streamsize read(Device& d, char* s, std::streamsize n);

	template<typename Device>
	std::streamsize write(Device& d, char* s, std::streamsize n);

	template<typename Device>
	void close(Device& d, std::ios_base::openmode mode);
};

struct lzw_compressor : boost::iostreams::multichar_dual_use_filter
{
	lzw_compressor(bool early_change) : trie(early_change), bits(0), num_bits(0), initial_cleared(false) {}
	template<typename Device>
	std::streamsize read(Device& d, char* s, std::streamsize n)
	{
		return n;
	}

	template<typename Device>
	std::streamsize write(Device& d, const char* s, std::streamsize n)
	{
		if(!initial_cleared) {
			trie.clear();
			put_bits(d, lzw_trie::make_value(trie.num_bits(), 256));
			initial_cleared = true;
		}
		const char* s_end = s + n;
		while(boost::optional<unsigned short> result = trie.step(s, s_end)) {
			put_bits(d, *result);
		}
		return n;
	}

	template<typename Device>
	void close(Device& d, std::ios_base::openmode mode)
	{
		if(mode & std::ios_base::in) {
		} else {
			while(boost::optional<unsigned short> result = trie.flush()) {
				put_bits(d, *result);
			}
			flush(d);
			initial_cleared = false;
		}
	}

private:

	template<typename Device>
	void put_bits_impl(Device& d)
	{
		while(buffer.size()) {
			unsigned short us = buffer.front();
			unsigned short nb = lzw_trie::bits(us);
			unsigned short b = lzw_trie::entry(us);
//std::cerr << static_cast<int>(num_bits) << ':' << static_cast<int>(bits) << ':' << nb << ':' << b << std::endl;
			while(num_bits + nb >= 8) {
				unsigned char c = bits << (8 - num_bits) | (b >> (nb + num_bits - 8)); // CHAR_BITS assumed as 8
				if(boost::iostreams::put(d, c)) {
					b = b & ((1U << (nb + num_bits - 8)) - 1);
					nb = nb + num_bits - 8;
					bits = num_bits = 0;
				} else {
					if(nb < 9) {
						bits = b;
						num_bits = nb;
						buffer.pop();
					} else {
						buffer.front() = lzw_trie::make_value(nb, b);
					}
					return;
				}
			}
			buffer.pop();
			bits = b;
			num_bits = nb;
		}
	}
	template<typename Device>
	void put_bits(Device &d, unsigned short us)
	{
		buffer.push(us);
		put_bits_impl(d);
	}
	template<typename Device>
	void flush(Device &d)
	{
		while(buffer.size()) put_bits_impl(d);
		if(num_bits) // rest and padding
			boost::iostreams::put(d, bits << (8 - num_bits));
		bits = num_bits = 0;
	}

	lzw_trie trie;
	std::queue<unsigned short> buffer;

	unsigned char bits;
	unsigned char num_bits;
	bool initial_cleared;
};

int main(void)
{
	using namespace boost::iostreams;

	std::vector<char> v;
	char input[11] = { 45, 45, 45, 45, 45, 65, 45, 45, 45, 66, 0 };
	char output[9] = { 0x80, 0x0b, 0x60, 0x50, 0x22, 0x0c, 0x0c, 0x85, 0x01 };

	// 1 0000 0000 : 1000 0000 : 0
	// 0 0010 1101 : 0000 1011 : 01
	// 1 0000 0010 : 0110 0000 : 010
	// 1 0000 0010 : 0101 0000 : 0010
	// 0 0100 0001 : 0010 0010 : 0 0001
	// 1 0000 0011 : 0000 1100 : 00 0011
	// 0 0100 0010 : 0000 1100 : 100 0010
	// 1 0000 0001 : 1000 0101 0000 0001 :

	// 1000 0000
	// 0000 1011
	// 0110 0000
	// 0101 0000
	// 0010 0010
	// 0000 1100
	// 0000 1100
	// 1000 0101
	// 0000 0001

	std::string in(input, input+10);
	std::string out(output, output+9);
	lzw_compressor comp(false);
	if(boost::iostreams::test_output_filter(comp, in, out)) {
		std::cout << "OK" << std::endl;
	} else {
		std::cout << "Error" << std::endl;
	}

	return 0;
}
