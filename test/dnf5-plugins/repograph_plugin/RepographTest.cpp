// Copyright Contributors to the DNF5 project.
// Copyright Contributors to the libdnf project.
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of libdnf: https://github.com/rpm-software-management/libdnf/
//
// Libdnf is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Libdnf is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with libdnf.  If not, see <https://www.gnu.org/licenses/>.

#include "RepographTest.hpp"

#include "repograph_graph.hpp"

#include <libdnf5/repo/repo_sack.hpp>
#include <libdnf5/rpm/package_query.hpp>
#include <libdnf5/rpm/package_sack.hpp>
#include <libdnf5/rpm/reldep_list.hpp>

#include <filesystem>
#include <initializer_list>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>


CPPUNIT_TEST_SUITE_REGISTRATION(RepographTest);


namespace {


libdnf5::rpm::PackageSet load_all_packages(
    BaseTestCase & fixture, std::initializer_list<const char *> repo_names = {"repograph-test"}) {
    for (const auto * repo_name : repo_names) {
        auto path = std::filesystem::path(TEST_DATADIR) / (std::string(repo_name) + ".repo");
        fixture.repo_sack->create_repo_from_libsolv_testcase(repo_name, path.string());
    }
    return libdnf5::rpm::PackageQuery(fixture.base);
}


struct RenderedGraph {
    std::string dot;
    dnf5::repograph::GraphStats stats;
};


RenderedGraph render_graph(const libdnf5::rpm::PackageSet & packages, bool include_recommends) {
    std::ostringstream out;
    auto stats = dnf5::repograph::emit_graph(out, packages, include_recommends);
    return {out.str(), stats};
}


void assert_graph(
    const std::string & dot,
    std::initializer_list<const char *> nodes,
    const std::vector<std::pair<const char *, const char *>> & edges) {
    std::ostringstream expected;
    expected << "digraph packages {\n";
    expected << "    node [shape=box, style=rounded, fontname=\"Helvetica\"];\n";
    for (const auto * node : nodes) {
        expected << "    " << std::quoted(node) << ";\n";
    }
    for (const auto & [from, to] : edges) {
        expected << "    " << std::quoted(from) << " -> " << std::quoted(to) << ";\n";
    }
    expected << "}\n";
    CPPUNIT_ASSERT_EQUAL(expected.str(), dot);
}


}  // namespace


void RepographTest::test_name_merging_and_dependency_types() {
    auto all = load_all_packages(*this);
    for (const auto include_recommends : {false, true}) {
        std::vector<std::pair<const char *, const char *>> edges{
            {"app", "liba"},
            {"app", "libb-impl1"},
            {"liba", "liba-common"},
            {"liba", "liba-i686"},
            {"liba", "liba-new-version"},
            {"liba", "liba-x86-64"}};
        if (include_recommends) {
            edges.insert(edges.begin(), {"app", "bonus"});
        }
        assert_graph(
            render_graph(all, include_recommends).dot,
            {"alternative-provider",
             "app",
             "bonus",
             "isolated",
             "liba",
             "liba-common",
             "liba-i686",
             "liba-new-version",
             "liba-x86-64",
             "libb-impl1",
             "libb-impl2",
             "self-provider",
             "suggestion"},
            edges);
    }
}


void RepographTest::test_provider_selection_is_deterministic() {
    auto all = load_all_packages(*this, {"repograph-providers-first", "repograph-providers-second"});
    BaseTestCase reverse_fixture;
    reverse_fixture.setUp();
    auto reverse_all = load_all_packages(reverse_fixture, {"repograph-providers-second", "repograph-providers-first"});
    for (const auto include_recommends : {false, true}) {
        const auto [dot, stats] = render_graph(all, include_recommends);
        assert_graph(dot, {"a-provider", "b-provider", "m-consumer", "z-provider"}, {{"m-consumer", "a-provider"}});
        CPPUNIT_ASSERT_EQUAL(dot, render_graph(reverse_all, include_recommends).dot);
        // Recommends reuses the Requires lookup without duplicating the edge.
        CPPUNIT_ASSERT_EQUAL(std::size_t{1}, stats.provider_cache_misses);
        CPPUNIT_ASSERT_EQUAL(std::size_t{include_recommends ? 6U : 5U}, stats.provider_cache_hits);
        CPPUNIT_ASSERT(!stats.has_rich_dependencies);
    }
    reverse_fixture.tearDown();
}


void RepographTest::test_self_provides_are_checked_per_package() {
    auto all = load_all_packages(*this, {"repograph-self"});
    for (const auto include_recommends : {false, true}) {
        assert_graph(
            render_graph(all, include_recommends).dot, {"a-source", "z-provider"}, {{"a-source", "z-provider"}});

        libdnf5::rpm::PackageQuery latest(all);
        latest.filter_latest_evr();
        assert_graph(render_graph(latest, include_recommends).dot, {"a-source", "z-provider"}, {});

        libdnf5::rpm::PackageQuery older(all);
        older.filter_nevra(std::vector<std::string>{"a-source-1.0-1.noarch", "z-provider-2.0-1.noarch"});
        assert_graph(
            render_graph(older, include_recommends).dot, {"a-source", "z-provider"}, {{"a-source", "z-provider"}});
    }
}


void RepographTest::test_versioned_requirements() {
    auto all = load_all_packages(*this, {"repograph-versioned"});
    const auto [dot, stats] = render_graph(all, true);
    assert_graph(
        dot,
        {"a-old-provider", "consumer-a", "consumer-b", "consumer-c", "z-new-provider"},
        {{"consumer-a", "z-new-provider"}, {"consumer-c", "z-new-provider"}});
    CPPUNIT_ASSERT_EQUAL(std::size_t{3}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{4}, stats.provider_cache_hits);
    CPPUNIT_ASSERT(!stats.has_rich_dependencies);
}


void RepographTest::test_provider_cache_reuses_missing_providers() {
    auto all = load_all_packages(*this, {"repograph-versioned"});
    libdnf5::rpm::PackageQuery nodes(all);
    nodes.filter_name({"consumer-a", "consumer-b", "consumer-c"});
    const auto [dot, stats] = render_graph(nodes, false);
    assert_graph(dot, {"consumer-a", "consumer-b", "consumer-c"}, {});
    CPPUNIT_ASSERT_EQUAL(std::size_t{3}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{4}, stats.provider_cache_hits);
    CPPUNIT_ASSERT(!stats.has_rich_dependencies);
}


void RepographTest::test_provider_cache_is_per_graph() {
    auto all = load_all_packages(*this, {"repograph-providers-first", "repograph-providers-second"});
    const auto full = render_graph(all, true);
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, full.stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{6}, full.stats.provider_cache_hits);

    libdnf5::rpm::PackageQuery nodes(all);
    nodes.filter_name(std::vector<std::string>{"m-consumer", "z-provider"});
    const auto limited = render_graph(nodes, true);
    assert_graph(limited.dot, {"m-consumer", "z-provider"}, {{"m-consumer", "z-provider"}});
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, limited.stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{2}, limited.stats.provider_cache_hits);

    nodes.filter_name("m-consumer");
    const auto missing = render_graph(nodes, true);
    assert_graph(missing.dot, {"m-consumer"}, {});
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, missing.stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, missing.stats.provider_cache_hits);

    const auto restored = render_graph(all, true);
    CPPUNIT_ASSERT_EQUAL(full.dot, restored.dot);
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, restored.stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{6}, restored.stats.provider_cache_hits);
}


void RepographTest::test_rich_and_requires_are_reported() {
    auto all = load_all_packages(*this, {"repograph-rich"});
    for (const auto * nevra : {"and-consumer-a-1.0-1.noarch", "and-consumer-b-1.0-1.noarch"}) {
        const auto dependencies = get_pkg(nevra).get_requires();
        CPPUNIT_ASSERT_EQUAL(1, dependencies.size());
        CPPUNIT_ASSERT_EQUAL(std::string("(rich-left and rich-right)"), dependencies.get(0).to_string());
    }
    libdnf5::rpm::PackageQuery nodes(all);
    nodes.filter_name({"and-consumer-a", "and-consumer-b", "rich-left", "rich-right"});
    const auto [dot, stats] = render_graph(nodes, false);
    // Libsolv returns potential providers of either operand, not a Boolean explanation.
    assert_graph(
        dot,
        {"and-consumer-a", "and-consumer-b", "rich-left", "rich-right"},
        {{"and-consumer-a", "rich-left"}, {"and-consumer-b", "rich-left"}});
    CPPUNIT_ASSERT(stats.has_rich_dependencies);
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, stats.provider_cache_hits);
}


void RepographTest::test_rich_conditional_requires_are_reported() {
    auto all = load_all_packages(*this, {"repograph-rich"});
    for (const auto * nevra : {"conditional-consumer-a-1.0-1.noarch", "conditional-consumer-b-1.0-1.noarch"}) {
        const auto dependencies = get_pkg(nevra).get_requires();
        CPPUNIT_ASSERT_EQUAL(1, dependencies.size());
        CPPUNIT_ASSERT_EQUAL(std::string("(rich-left if rich-condition)"), dependencies.get(0).to_string());
    }
    libdnf5::rpm::PackageQuery nodes(all);
    nodes.filter_name({"conditional-consumer-a", "conditional-consumer-b", "rich-condition", "rich-left"});
    const auto [dot, stats] = render_graph(nodes, false);
    assert_graph(
        dot,
        {"conditional-consumer-a", "conditional-consumer-b", "rich-condition", "rich-left"},
        {{"conditional-consumer-a", "rich-left"}, {"conditional-consumer-b", "rich-left"}});
    CPPUNIT_ASSERT(stats.has_rich_dependencies);
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{1}, stats.provider_cache_hits);

    // Provider approximation does not evaluate whether the condition is active.
    nodes.filter_name("rich-condition", libdnf5::sack::QueryCmp::NEQ);
    assert_graph(
        render_graph(nodes, false).dot,
        {"conditional-consumer-a", "conditional-consumer-b", "rich-left"},
        {{"conditional-consumer-a", "rich-left"}, {"conditional-consumer-b", "rich-left"}});
}


void RepographTest::test_rich_dependency_cache_reuse() {
    auto all = load_all_packages(*this, {"repograph-rich"});
    const auto [dot, stats] = render_graph(all, true);
    assert_graph(
        dot,
        {"and-consumer-a",
         "and-consumer-b",
         "conditional-consumer-a",
         "conditional-consumer-b",
         "rich-condition",
         "rich-left",
         "rich-recommender",
         "rich-right",
         "rich-suggester"},
        {{"and-consumer-a", "rich-left"},
         {"and-consumer-b", "rich-left"},
         {"conditional-consumer-a", "rich-left"},
         {"conditional-consumer-b", "rich-left"},
         {"rich-recommender", "rich-left"}});
    CPPUNIT_ASSERT(stats.has_rich_dependencies);
    CPPUNIT_ASSERT_EQUAL(std::size_t{2}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{4}, stats.provider_cache_hits);
}


void RepographTest::test_rich_recommends_follow_configuration() {
    auto all = load_all_packages(*this, {"repograph-rich"});
    const auto dependencies = get_pkg("rich-recommender-1.0-1.noarch").get_recommends();
    CPPUNIT_ASSERT_EQUAL(2, dependencies.size());
    CPPUNIT_ASSERT_EQUAL(std::string("(rich-left and rich-right)"), dependencies.get(0).to_string());
    CPPUNIT_ASSERT_EQUAL(std::string("(rich-left if rich-condition)"), dependencies.get(1).to_string());
    libdnf5::rpm::PackageQuery nodes(all);
    nodes.filter_name({"rich-condition", "rich-left", "rich-recommender", "rich-right"});
    const auto enabled = render_graph(nodes, true);
    assert_graph(
        enabled.dot,
        {"rich-condition", "rich-left", "rich-recommender", "rich-right"},
        {{"rich-recommender", "rich-left"}});
    CPPUNIT_ASSERT(enabled.stats.has_rich_dependencies);
    CPPUNIT_ASSERT_EQUAL(std::size_t{2}, enabled.stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{0}, enabled.stats.provider_cache_hits);

    const auto disabled = render_graph(nodes, false);
    assert_graph(disabled.dot, {"rich-condition", "rich-left", "rich-recommender", "rich-right"}, {});
    CPPUNIT_ASSERT(!disabled.stats.has_rich_dependencies);
    CPPUNIT_ASSERT_EQUAL(std::size_t{0}, disabled.stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{0}, disabled.stats.provider_cache_hits);
}


void RepographTest::test_rich_suggests_are_not_reported() {
    auto all = load_all_packages(*this, {"repograph-rich"});
    const auto dependencies = get_pkg("rich-suggester-1.0-1.noarch").get_suggests();
    CPPUNIT_ASSERT_EQUAL(1, dependencies.size());
    CPPUNIT_ASSERT_EQUAL(std::string("(rich-left and rich-right)"), dependencies.get(0).to_string());
    libdnf5::rpm::PackageQuery nodes(all);
    nodes.filter_name({"rich-condition", "rich-left", "rich-right", "rich-suggester"});
    const auto [dot, stats] = render_graph(nodes, true);
    assert_graph(dot, {"rich-condition", "rich-left", "rich-right", "rich-suggester"}, {});
    CPPUNIT_ASSERT(!stats.has_rich_dependencies);
    CPPUNIT_ASSERT_EQUAL(std::size_t{0}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{0}, stats.provider_cache_hits);
}


void RepographTest::test_rich_dependencies_stay_in_node_set() {
    auto all = load_all_packages(*this, {"repograph-rich"});
    libdnf5::rpm::PackageQuery nodes(all);
    nodes.filter_name({"and-consumer-a", "conditional-consumer-a", "rich-condition", "rich-recommender", "rich-right"});
    assert_graph(
        render_graph(nodes, true).dot,
        {"and-consumer-a", "conditional-consumer-a", "rich-condition", "rich-recommender", "rich-right"},
        {{"and-consumer-a", "rich-right"}, {"rich-recommender", "rich-right"}});
    nodes.filter_name(std::vector<std::string>{"rich-condition", "rich-right"}, libdnf5::sack::QueryCmp::NEQ);
    const auto [dot, stats] = render_graph(nodes, true);
    assert_graph(dot, {"and-consumer-a", "conditional-consumer-a", "rich-recommender"}, {});
    CPPUNIT_ASSERT(stats.has_rich_dependencies);
    CPPUNIT_ASSERT_EQUAL(std::size_t{2}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{2}, stats.provider_cache_hits);
}


void RepographTest::test_emits_empty_graph() {
    libdnf5::rpm::PackageSet empty(base);
    const auto [dot, stats] = render_graph(empty, true);
    assert_graph(dot, {}, {});
    CPPUNIT_ASSERT_EQUAL(std::size_t{0}, stats.provider_cache_misses);
    CPPUNIT_ASSERT_EQUAL(std::size_t{0}, stats.provider_cache_hits);
    CPPUNIT_ASSERT(!stats.has_rich_dependencies);
}
