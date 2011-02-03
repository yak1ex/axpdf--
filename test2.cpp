#include <boost/spirit/include/qi.hpp>
#include <string>

int main()
{
	namespace qi = boost::spirit::qi;

	std::string str;
	typedef std::string::iterator Iterator;
	Iterator first = str.begin(), last = str.end();

	qi::rule<Iterator, qi::space_type> rule_with_skip, rule_with_skip2, rule_with_skip3;
	qi::rule<Iterator> rule_without_skip, rule_without_skip2;

	rule_with_skip    = qi::int_;
	rule_without_skip = qi::int_;

	phrase_parse(first, last, rule_with_skip,    qi::space);
	phrase_parse(first, last, rule_without_skip, qi::space); // CAN    COMPILE!!
//	phrase_parse(first, last, rule_with_skip,    qi::cntrl); // CANNOT COMPILE!!
	phrase_parse(first, last, rule_without_skip, qi::cntrl); // CAN    COMPILE!!
//	parse       (first, last, rule_with_skip);               // CANNOT COMPILE!!
	parse       (first, last, rule_without_skip);

	rule_with_skip2    = rule_with_skip >> rule_without_skip; // CAN    COMPILE!!
//	rule_without_skip2 = rule_with_skip >> rule_without_skip; // CANNOT COMPILE!!
	rule_with_skip2    = rule_with_skip >> qi::no_skip[rule_without_skip]; // CAN COMPILE!!
	rule_without_skip2 = qi::skip(qi::space)[rule_with_skip] >> rule_without_skip; // CAN COMPILE!!

	rule_with_skip2    = qi::no_skip[qi::skip[rule_with_skip]]; // CAN COMPILE!!
	rule_with_skip2    = qi::no_skip[rule_without_skip2];
//	rule_without_skip2 = qi::skip[rule_with_skip];              // CANNOT COMPILE!!
	rule_without_skip2 = qi::skip(qi::space)[rule_with_skip];   // CANNOT COMPILE!!

	return 0;
}
