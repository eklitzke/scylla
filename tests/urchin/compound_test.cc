/*
 * Copyright 2015 Cloudius Systems
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE core

#include <boost/test/unit_test.hpp>
#include "compound.hh"
#include "compound_compat.hh"
#include "tests/urchin/range_assert.hh"

static std::vector<bytes> to_bytes_vec(std::vector<sstring> values) {
    std::vector<bytes> result;
    for (auto&& v : values) {
        result.emplace_back(to_bytes(v));
    }
    return result;
}

template <typename Compound>
static
range_assert<typename Compound::iterator>
assert_that_components(Compound& t, bytes packed) {
    return assert_that_range(t.begin(packed), t.end(packed));
}

template <typename Compound>
static void test_sequence(Compound& t, std::vector<sstring> strings) {
    auto packed = t.serialize_value(to_bytes_vec(strings));
    assert_that_components(t, packed).equals(to_bytes_vec(strings));
};

BOOST_AUTO_TEST_CASE(test_iteration_over_non_prefixable_tuple) {
    compound_type<allow_prefixes::no> t({bytes_type, bytes_type, bytes_type});

    test_sequence(t, {"el1", "el2", "el3"});
    test_sequence(t, {"el1", "el2", ""});
    test_sequence(t, {"",    "el2", "el3"});
    test_sequence(t, {"el1", "",    ""});
    test_sequence(t, {"",    "",    "el3"});
    test_sequence(t, {"el1", "",    "el3"});
    test_sequence(t, {"",    "",    ""});
}

BOOST_AUTO_TEST_CASE(test_iteration_over_prefixable_tuple) {
    compound_type<allow_prefixes::yes> t({bytes_type, bytes_type, bytes_type});

    test_sequence(t, {"el1", "el2", "el3"});
    test_sequence(t, {"el1", "el2", ""});
    test_sequence(t, {"",    "el2", "el3"});
    test_sequence(t, {"el1", "",    ""});
    test_sequence(t, {"",    "",    "el3"});
    test_sequence(t, {"el1", "",    "el3"});
    test_sequence(t, {"",    "",    ""});

    test_sequence(t, {"el1", "el2", ""});
    test_sequence(t, {"el1", "el2"});
    test_sequence(t, {"el1", ""});
    test_sequence(t, {"el1"});
    test_sequence(t, {""});
    test_sequence(t, {});
}

BOOST_AUTO_TEST_CASE(test_iteration_over_non_prefixable_singular_tuple) {
    compound_type<allow_prefixes::no> t({bytes_type});

    test_sequence(t, {"el1"});
    test_sequence(t, {""});
}

BOOST_AUTO_TEST_CASE(test_iteration_over_prefixable_singular_tuple) {
    compound_type<allow_prefixes::yes> t({bytes_type});

    test_sequence(t, {"elem1"});
    test_sequence(t, {""});
    test_sequence(t, {});
}

template <allow_prefixes AllowPrefixes>
void do_test_conversion_methods_for_singular_compound() {
    compound_type<AllowPrefixes> t({bytes_type});

    {
        assert_that_components(t, t.serialize_value(to_bytes_vec({"asd"}))) // r-value version
            .equals(to_bytes_vec({"asd"}));
    }

    {
        auto vec = to_bytes_vec({"asd"});
        assert_that_components(t, t.serialize_value(vec)) // l-value version
            .equals(to_bytes_vec({"asd"}));
    }

    {
        std::vector<bytes_opt> vec = { bytes_opt("asd") };
        assert_that_components(t, t.serialize_optionals(vec))
            .equals(to_bytes_vec({"asd"}));
    }

    {
        std::vector<bytes_opt> vec = { bytes_opt("asd") };
        assert_that_components(t, t.serialize_optionals(std::move(vec))) // r-value
            .equals(to_bytes_vec({"asd"}));
    }

    {
        assert_that_components(t, t.serialize_single(bytes("asd")))
            .equals(to_bytes_vec({"asd"}));
    }
}

BOOST_AUTO_TEST_CASE(test_conversion_methods_for_singular_compound) {
    do_test_conversion_methods_for_singular_compound<allow_prefixes::yes>();
    do_test_conversion_methods_for_singular_compound<allow_prefixes::no>();
}

template <allow_prefixes AllowPrefixes>
void do_test_conversion_methods_for_non_singular_compound() {
    compound_type<AllowPrefixes> t({bytes_type, bytes_type, bytes_type});

    {
        assert_that_components(t, t.serialize_value(to_bytes_vec({"el1", "el2", "el2"}))) // r-value version
            .equals(to_bytes_vec({"el1", "el2", "el2"}));
    }

    {
        auto vec = to_bytes_vec({"el1", "el2", "el3"});
        assert_that_components(t, t.serialize_value(vec)) // l-value version
            .equals(to_bytes_vec({"el1", "el2", "el3"}));
    }

    {
        std::vector<bytes_opt> vec = { bytes_opt("el1"), bytes_opt("el2"), bytes_opt("el3") };
        assert_that_components(t, t.serialize_optionals(vec))
            .equals(to_bytes_vec({"el1", "el2", "el3"}));
    }

    {
        std::vector<bytes_opt> vec = { bytes_opt("el1"), bytes_opt("el2"), bytes_opt("el3") };
        assert_that_components(t, t.serialize_optionals(std::move(vec))) // r-value
            .equals(to_bytes_vec({"el1", "el2", "el3"}));
    }
}

BOOST_AUTO_TEST_CASE(test_conversion_methods_for_non_singular_compound) {
    do_test_conversion_methods_for_non_singular_compound<allow_prefixes::yes>();
    do_test_conversion_methods_for_non_singular_compound<allow_prefixes::no>();
}

BOOST_AUTO_TEST_CASE(test_component_iterator_post_incrementation) {
    compound_type<allow_prefixes::no> t({bytes_type, bytes_type, bytes_type});

    auto packed = t.serialize_value(to_bytes_vec({"el1", "el2", "el3"}));
    auto i = t.begin(packed);
    auto end = t.end(packed);
    BOOST_REQUIRE_EQUAL(to_bytes("el1"), *i++);
    BOOST_REQUIRE_EQUAL(to_bytes("el2"), *i++);
    BOOST_REQUIRE_EQUAL(to_bytes("el3"), *i++);
    BOOST_REQUIRE(i == end);
}

BOOST_AUTO_TEST_CASE(test_conversion_to_legacy_form) {
    compound_type<allow_prefixes::no> singular({bytes_type});

    BOOST_REQUIRE_EQUAL(to_legacy(singular, singular.serialize_single(to_bytes("asd"))), bytes("asd"));
    BOOST_REQUIRE_EQUAL(to_legacy(singular, singular.serialize_single(to_bytes(""))), bytes(""));

    compound_type<allow_prefixes::no> two_components({bytes_type, bytes_type});

    BOOST_REQUIRE_EQUAL(to_legacy(two_components, two_components.serialize_value(to_bytes_vec({"el1", "elem2"}))),
        bytes({'\x00', '\x03', 'e', 'l', '1', '\x00', '\x00', '\x05', 'e', 'l', 'e', 'm', '2', '\x00'}));

    BOOST_REQUIRE_EQUAL(to_legacy(two_components, two_components.serialize_value(to_bytes_vec({"el1", ""}))),
        bytes({'\x00', '\x03', 'e', 'l', '1', '\x00', '\x00', '\x00', '\x00'}));
}

BOOST_AUTO_TEST_CASE(test_legacy_ordering_of_singular) {
    compound_type<allow_prefixes::no> t({bytes_type});

    auto make = [&t] (sstring value) -> bytes {
        return t.serialize_single(to_bytes(value));
    };

    legacy_compound_view<decltype(t)>::tri_comparator cmp(t);

    BOOST_REQUIRE(cmp(make("A"), make("B"))  < 0);
    BOOST_REQUIRE(cmp(make("AA"), make("B")) < 0);
    BOOST_REQUIRE(cmp(make("B"), make("AB")) > 0);
    BOOST_REQUIRE(cmp(make("B"), make("A"))  > 0);
    BOOST_REQUIRE(cmp(make("A"), make("A")) == 0);
}

BOOST_AUTO_TEST_CASE(test_legacy_ordering_of_composites) {
    compound_type<allow_prefixes::no> t({bytes_type, bytes_type});

    auto make = [&t] (sstring v1, sstring v2) -> bytes {
        return t.serialize_value(std::vector<bytes>{to_bytes(v1), to_bytes(v2)});
    };

    legacy_compound_view<decltype(t)>::tri_comparator cmp(t);

    BOOST_REQUIRE(cmp(make("A", "B"), make("A", "B")) == 0);
    BOOST_REQUIRE(cmp(make("A", "B"), make("A", "C")) < 0);
    BOOST_REQUIRE(cmp(make("A", "B"), make("B", "B")) < 0);
    BOOST_REQUIRE(cmp(make("A", "C"), make("B", "B")) < 0);
    BOOST_REQUIRE(cmp(make("B", "A"), make("A", "A")) > 0);

    BOOST_REQUIRE(cmp(make("AA", "B"), make("B", "B")) > 0);
    BOOST_REQUIRE(cmp(make("A", "AA"), make("A", "A")) > 0);

    BOOST_REQUIRE(cmp(make("", "A"), make("A", "A")) < 0);
    BOOST_REQUIRE(cmp(make("A", ""), make("A", "A")) < 0);
}