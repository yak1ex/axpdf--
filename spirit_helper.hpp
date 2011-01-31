#ifndef YAK_SPIRIT_HELPER_HPP
#define YAK_SPIRIT_HELPER_HPP

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/typeof/typeof.hpp>

#ifndef BOOST_SPIRIT_AUTO
// Extracted from http://svn.boost.org/svn/boost/trunk/libs/spirit/example/qi/typeof.cpp@68572
#define BOOST_SPIRIT_AUTO(domain_, name, expr)                                  \
    typedef boost::proto::result_of::                                           \
        deep_copy<BOOST_TYPEOF(expr)>::type name##_expr_type;                   \
    BOOST_SPIRIT_ASSERT_MATCH(                                                  \
        boost::spirit::domain_::domain, name##_expr_type);                      \
    BOOST_AUTO(name, boost::proto::deep_copy(expr));                            \

#endif /* BOOST_SPIRIT_AUTO */

#endif /* YAK_SPIRIT_HELPER_HPP */
