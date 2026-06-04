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

#include <filesystem>
#include <iomanip>
#include <sstream>


CPPUNIT_TEST_SUITE_REGISTRATION(RepographTest);


namespace {


/// Load the fixture libsolv testcase repo and return all packages in
/// the sack as a PackageSet. Centralised so individual tests pick the
/// subset they care about.
libdnf5::rpm::PackageSet load_all_packages(BaseTestCase & fixture) {
    auto path = std::filesystem::path(TEST_DATADIR) / "repograph-test.repo";
    fixture.repo_sack->create_repo_from_libsolv_testcase("repograph-test", path.string());
    return libdnf5::rpm::PackageQuery(fixture.base);
}


std::string render_graph(const libdnf5::rpm::PackageSet & packages, bool include_recommends) {
    std::ostringstream out;
    dnf5::repograph::emit_graph(out, packages, include_recommends);
    return out.str();
}


std::string node_statement(const std::string & name) {
    std::ostringstream out;
    out << std::quoted(name) << ';';
    return out.str();
}


std::string edge_statement(const std::string & from, const std::string & to) {
    std::ostringstream out;
    out << std::quoted(from) << " -> " << std::quoted(to) << ';';
    return out.str();
}


bool contains_statement(const std::string & dot, const std::string & statement) {
    std::istringstream input(dot);
    for (std::string line; std::getline(input, line);) {
        line.erase(0, line.find_first_not_of(" \t"));
        if (line == statement) {
            return true;
        }
    }
    return false;
}


bool contains_package_statement(const std::string & dot) {
    std::istringstream input(dot);
    for (std::string line; std::getline(input, line);) {
        const auto first_non_whitespace = line.find_first_not_of(" \t");
        if (first_non_whitespace != std::string::npos && line[first_non_whitespace] == '"') {
            return true;
        }
    }
    return false;
}


bool has_node(const std::string & dot, const std::string & name) {
    return contains_statement(dot, node_statement(name));
}


bool has_edge(const std::string & dot, const std::string & from, const std::string & to) {
    return contains_statement(dot, edge_statement(from, to));
}


void assert_has_node(const std::string & dot, const std::string & name) {
    CPPUNIT_ASSERT_MESSAGE("Expected node \"" + name + "\" in dot output", has_node(dot, name));
}


void assert_lacks_node(const std::string & dot, const std::string & name) {
    CPPUNIT_ASSERT_MESSAGE("Unexpected node \"" + name + "\" in dot output", !has_node(dot, name));
}


void assert_has_edge(const std::string & dot, const std::string & from, const std::string & to) {
    CPPUNIT_ASSERT_MESSAGE("Expected edge \"" + from + "\" -> \"" + to + "\" in dot output", has_edge(dot, from, to));
}


void assert_lacks_edge(const std::string & dot, const std::string & from, const std::string & to) {
    CPPUNIT_ASSERT_MESSAGE(
        "Unexpected edge \"" + from + "\" -> \"" + to + "\" in dot output", !has_edge(dot, from, to));
}


}  // namespace


void RepographTest::test_emits_graph_document() {
    auto dot = render_graph(load_all_packages(*this), true);
    CPPUNIT_ASSERT(dot.starts_with("digraph packages {\n"));
    // The output is a complete dot document with a trailing newline.
    CPPUNIT_ASSERT(dot.ends_with("}\n"));
}


void RepographTest::test_nodes_are_name_merged() {
    auto dot = render_graph(load_all_packages(*this), true);
    // The fixture defines liba in two arches (x86_64, i686). Name-based
    // merging must collapse them into a single "liba" node declaration;
    // the arch must not appear in any node id.
    assert_has_node(dot, "liba");
    assert_lacks_node(dot, "liba-1.2-1.x86_64");
    assert_lacks_node(dot, "liba.x86_64");
}


void RepographTest::test_strong_required_edges_present() {
    auto dot = render_graph(load_all_packages(*this), true);
    assert_has_edge(dot, "app", "liba");
}


void RepographTest::test_multiple_providers_are_deterministic() {
    auto dot = render_graph(load_all_packages(*this), true);
    assert_has_edge(dot, "app", "libb-impl1");
    assert_lacks_edge(dot, "app", "libb-impl2");
}


void RepographTest::test_recommends_excluded() {
    auto dot = render_graph(load_all_packages(*this), false);
    assert_lacks_edge(dot, "app", "bonus");
    assert_has_edge(dot, "app", "liba");
}


void RepographTest::test_recommends_included() {
    auto dot = render_graph(load_all_packages(*this), true);
    assert_has_edge(dot, "app", "bonus");
}


void RepographTest::test_suggests_excluded() {
    auto dot = render_graph(load_all_packages(*this), true);
    assert_lacks_edge(dot, "app", "suggestion");
}


void RepographTest::test_no_self_edges() {
    auto dot = render_graph(load_all_packages(*this), true);
    for (const auto & name : {"app", "liba", "libb-impl1", "libb-impl2", "bonus", "suggestion", "self-provider"}) {
        assert_lacks_edge(dot, name, name);
    }
}


void RepographTest::test_self_provider_does_not_hide_alternative() {
    auto dot = render_graph(load_all_packages(*this), true);
    assert_has_edge(dot, "self-provider", "alternative-provider");
}


void RepographTest::test_no_edges_to_missing_providers() {
    auto dot = render_graph(load_all_packages(*this), true);
    assert_lacks_node(dot, "optional-pkg");
    assert_lacks_edge(dot, "app", "optional-pkg");
}


void RepographTest::test_output_is_deterministic() {
    auto all = load_all_packages(*this);
    CPPUNIT_ASSERT_EQUAL(render_graph(all, true), render_graph(all, true));
}


void RepographTest::test_emits_empty_graph() {
    libdnf5::rpm::PackageSet empty(base);
    auto dot = render_graph(empty, true);
    CPPUNIT_ASSERT(dot.starts_with("digraph packages {\n"));
    CPPUNIT_ASSERT(dot.ends_with("}\n"));
    CPPUNIT_ASSERT_MESSAGE("Empty graph contains a package statement", !contains_package_statement(dot));
}
