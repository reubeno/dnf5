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

#include "dot_emitter.hpp"
#include "graph_builder.hpp"
#include "json_emitter.hpp"
#include "repograph.hpp"

#include <dnf5/context.hpp>

#include <sstream>


using namespace dnf5;


namespace {


/// Create an argument-parser-ready context and a RepographCommand. The
/// returned command has had `set_argument_parser()` called, so its
/// option/argument metadata is queryable.
struct Fixture {
    std::vector<std::unique_ptr<libdnf5::Logger>> loggers;
    std::unique_ptr<Context> ctx;
    std::unique_ptr<RepographCommand> cmd;

    Fixture() {
        ctx = std::make_unique<Context>(std::move(loggers));
        auto & parser = ctx->get_argument_parser();
        auto root = parser.add_new_command("test");
        parser.set_root_command(root);
        cmd = std::make_unique<RepographCommand>(*ctx);
        cmd->set_argument_parser();
    }
};


repograph::Graph make_two_node_graph() {
    repograph::Node a;
    a.id = "a";
    a.name = "a";
    a.epoch = "0";
    a.version = "1";
    a.release = "1";
    a.arch = "x86_64";
    a.repo = "test";
    a.nevra = "a-1-1.x86_64";

    repograph::Node b;
    b.id = "b";
    b.name = "b";
    b.epoch = "";
    b.version = "2";
    b.release = "3";
    b.arch = "x86_64";
    b.repo = "test";
    b.nevra = "b-2-3.x86_64";

    repograph::Edge e;
    e.from = "a";
    e.to = "b";
    repograph::EdgeReldep rd;
    rd.reldep = "libb.so.1()(64bit)";
    rd.kind = repograph::EdgeKind::REQUIRES;
    e.reldeps.push_back(rd);

    repograph::Graph g;
    g.nodes = {a, b};
    g.edges = {e};
    return g;
}


}  // namespace


void RepographTest::setUp() {}
void RepographTest::tearDown() {}


void RepographTest::test_all_options_registered() {
    Fixture f;
    auto & cp = *f.cmd->get_argument_parser_command();

    // Each of these throws if the arg isn't registered.
    (void)cp.get_named_arg("use-system");
    (void)cp.get_named_arg("closed");
    (void)cp.get_named_arg("include-reverse-weak");
    (void)cp.get_named_arg("resolver");
    (void)cp.get_named_arg("node-label");
    (void)cp.get_named_arg("edge-label");
    (void)cp.get_named_arg("format");
    (void)cp.get_named_arg("output");
    (void)cp.get_named_arg("json");
    (void)cp.get_positional_arg("specs");
}


void RepographTest::test_use_system_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("use-system");
    CPPUNIT_ASSERT_EQUAL(std::string("use-system"), arg.get_long_name());
    CPPUNIT_ASSERT(!arg.get_description().empty());
}


void RepographTest::test_closed_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("closed");
    CPPUNIT_ASSERT_EQUAL(std::string("closed"), arg.get_long_name());
    std::string desc = arg.get_description();
    CPPUNIT_ASSERT(desc.find("SPEC") != std::string::npos);
    CPPUNIT_ASSERT(desc.find("--use-system") != std::string::npos);
}


void RepographTest::test_include_reverse_weak_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("include-reverse-weak");
    CPPUNIT_ASSERT_EQUAL(std::string("include-reverse-weak"), arg.get_long_name());
    std::string desc = arg.get_description();
    CPPUNIT_ASSERT(desc.find("Supplements") != std::string::npos);
}


void RepographTest::test_resolver_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("resolver");
    CPPUNIT_ASSERT_EQUAL(std::string("resolver"), arg.get_long_name());
    CPPUNIT_ASSERT(arg.get_has_value());
}


void RepographTest::test_node_label_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("node-label");
    CPPUNIT_ASSERT_EQUAL(std::string("node-label"), arg.get_long_name());
    CPPUNIT_ASSERT(arg.get_has_value());
}


void RepographTest::test_edge_label_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("edge-label");
    CPPUNIT_ASSERT_EQUAL(std::string("edge-label"), arg.get_long_name());
    CPPUNIT_ASSERT(arg.get_has_value());
}


void RepographTest::test_format_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("format");
    CPPUNIT_ASSERT_EQUAL(std::string("format"), arg.get_long_name());
    CPPUNIT_ASSERT(arg.get_has_value());
}


void RepographTest::test_output_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("output");
    CPPUNIT_ASSERT_EQUAL(std::string("output"), arg.get_long_name());
    CPPUNIT_ASSERT(arg.get_has_value());
}


void RepographTest::test_json_option() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_named_arg("json");
    CPPUNIT_ASSERT_EQUAL(std::string("json"), arg.get_long_name());
}


void RepographTest::test_specs_positional() {
    Fixture f;
    auto & arg = f.cmd->get_argument_parser_command()->get_positional_arg("specs");
    CPPUNIT_ASSERT(!arg.get_description().empty());
}


void RepographTest::test_default_markers_in_help() {
    Fixture f;
    auto & cp = *f.cmd->get_argument_parser_command();
    // dnf5 marks defaults inline in the description text. Verify our
    // enum-valued options follow that convention.
    CPPUNIT_ASSERT(cp.get_named_arg("resolver").get_description().find("default: solver") != std::string::npos);
    CPPUNIT_ASSERT(cp.get_named_arg("node-label").get_description().find("default: name") != std::string::npos);
    CPPUNIT_ASSERT(cp.get_named_arg("edge-label").get_description().find("default: none") != std::string::npos);
    CPPUNIT_ASSERT(cp.get_named_arg("format").get_description().find("default: dot") != std::string::npos);
}


void RepographTest::test_edge_kind_strings() {
    CPPUNIT_ASSERT_EQUAL(std::string("requires"), repograph::to_string(repograph::EdgeKind::REQUIRES));
    CPPUNIT_ASSERT_EQUAL(std::string("requires-pre"), repograph::to_string(repograph::EdgeKind::REQUIRES_PRE));
    CPPUNIT_ASSERT_EQUAL(std::string("recommends"), repograph::to_string(repograph::EdgeKind::RECOMMENDS));
    CPPUNIT_ASSERT_EQUAL(std::string("suggests"), repograph::to_string(repograph::EdgeKind::SUGGESTS));
    CPPUNIT_ASSERT_EQUAL(std::string("supplemented-by"), repograph::to_string(repograph::EdgeKind::SUPPLEMENTED_BY));
    CPPUNIT_ASSERT_EQUAL(std::string("enhanced-by"), repograph::to_string(repograph::EdgeKind::ENHANCED_BY));
}


void RepographTest::test_dot_emit_empty() {
    repograph::Graph g;
    std::ostringstream out;
    repograph::emit_dot(out, g, repograph::EdgeAnnotations::BOTH);
    std::string s = out.str();
    CPPUNIT_ASSERT(s.find("digraph packages {") != std::string::npos);
    CPPUNIT_ASSERT(s.rfind("}\n") != std::string::npos);
}


void RepographTest::test_dot_emit_simple() {
    auto g = make_two_node_graph();
    std::ostringstream out;
    repograph::emit_dot(out, g, repograph::EdgeAnnotations::BOTH);
    std::string s = out.str();
    CPPUNIT_ASSERT(s.find("\"a\";") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"b\";") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"a\" -> \"b\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("requires:libb.so.1") != std::string::npos);
}


void RepographTest::test_dot_emit_quoting() {
    repograph::Graph g;
    repograph::Node n;
    n.id = "weird\"id\\here";
    g.nodes.push_back(n);
    std::ostringstream out;
    repograph::emit_dot(out, g, repograph::EdgeAnnotations::NONE);
    std::string s = out.str();
    CPPUNIT_ASSERT(s.find("\"weird\\\"id\\\\here\"") != std::string::npos);
}


void RepographTest::test_dot_emit_annotations_modes() {
    auto g = make_two_node_graph();

    auto render = [&](repograph::EdgeAnnotations mode) {
        std::ostringstream out;
        repograph::emit_dot(out, g, mode);
        return out.str();
    };

    std::string both = render(repograph::EdgeAnnotations::BOTH);
    CPPUNIT_ASSERT(both.find("requires:libb.so.1") != std::string::npos);

    std::string reldep_only = render(repograph::EdgeAnnotations::RELDEP);
    CPPUNIT_ASSERT(reldep_only.find("libb.so.1") != std::string::npos);
    CPPUNIT_ASSERT(reldep_only.find("requires:") == std::string::npos);

    std::string kind_only = render(repograph::EdgeAnnotations::KIND);
    CPPUNIT_ASSERT(kind_only.find("label=\"requires\"") != std::string::npos);
    CPPUNIT_ASSERT(kind_only.find("libb.so.1") == std::string::npos);

    std::string none = render(repograph::EdgeAnnotations::NONE);
    CPPUNIT_ASSERT(none.find("label=") == std::string::npos);
}


void RepographTest::test_json_emit_empty() {
    repograph::Graph g;
    std::ostringstream out;
    repograph::emit_json(out, g, "repo-wide", "nevra", "best");
    std::string s = out.str();
    CPPUNIT_ASSERT(s.find("\"mode\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"repo-wide\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"nodes\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"edges\"") != std::string::npos);
}


void RepographTest::test_json_emit_simple() {
    auto g = make_two_node_graph();
    std::ostringstream out;
    repograph::emit_json(out, g, "targeted", "nevra", "solver");
    std::string s = out.str();
    CPPUNIT_ASSERT(s.find("\"targeted\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"a-1-1.x86_64\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"libb.so.1()(64bit)\"") != std::string::npos);
    // Edge "from"/"to" should refer to display ids ("a","b") not nevras.
    CPPUNIT_ASSERT(s.find("\"from\": \"a\"") != std::string::npos || s.find("\"from\":\"a\"") != std::string::npos);
    // Edge dependency list uses user-friendly "deps"/"dep" field names,
    // not the internal "reldeps"/"reldep" jargon.
    CPPUNIT_ASSERT(s.find("\"deps\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"dep\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("\"reldeps\"") == std::string::npos);
}


void RepographTest::test_json_emit_includes_members() {
    repograph::Graph g;
    repograph::Node merged;
    merged.id = "foo";
    merged.name = "foo";
    merged.version = "1";
    merged.release = "1";
    merged.arch = "x86_64";
    merged.repo = "test";
    merged.nevra = "foo-1-1.x86_64";
    merged.members = {"foo-1-1.i686", "foo-1-1.x86_64", "foo-2-1.x86_64"};
    g.nodes.push_back(merged);

    std::ostringstream out;
    repograph::emit_json(out, g, "repo-wide", "name", "best");
    std::string s = out.str();
    CPPUNIT_ASSERT(s.find("\"members\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("foo-1-1.i686") != std::string::npos);
    CPPUNIT_ASSERT(s.find("foo-2-1.x86_64") != std::string::npos);
}


void RepographTest::test_json_emit_includes_unresolved() {
    repograph::Graph g;
    repograph::Node n;
    n.id = "bash";
    n.name = "bash";
    n.version = "5.3.9";
    n.release = "5";
    n.arch = "x86_64";
    n.repo = "test";
    n.nevra = "bash-5.3.9-5.x86_64";

    repograph::EdgeReldep u1;
    u1.reldep = "libc.so.6()(64bit)";
    u1.kind = repograph::EdgeKind::REQUIRES;
    repograph::EdgeReldep u2;
    u2.reldep = "libtinfo.so.6()(64bit)";
    u2.kind = repograph::EdgeKind::REQUIRES;
    n.unresolved = {u1, u2};

    g.nodes.push_back(n);

    std::ostringstream out;
    repograph::emit_json(out, g, "specs-closed", "name", "solver");
    std::string s = out.str();
    CPPUNIT_ASSERT(s.find("\"unresolved\"") != std::string::npos);
    CPPUNIT_ASSERT(s.find("libc.so.6") != std::string::npos);
    CPPUNIT_ASSERT(s.find("libtinfo.so.6") != std::string::npos);
}


CPPUNIT_TEST_SUITE_REGISTRATION(RepographTest);
