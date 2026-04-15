#include <tavros/tef/loader.hpp>

#include <tavros/core/containers/fixed_vector.hpp>
#include <tavros/core/logger/logger.hpp>
#include <tavros/core/debug/unreachable.hpp>
#include <tavros/core/utils/string_hash.hpp>
#include <tavros/core/exception.hpp>

#include <tavros/tef/parser.hpp>

#include <string>

namespace
{
    tavros::core::logger logger("tef_loader");

    tavros::core::string_view to_string(tavros::tef::node::node_type type) noexcept
    {
        switch (type) {
        case tavros::tef::node::node_type::document:
            return "document";
        case tavros::tef::node::node_type::object:
            return "object";
        case tavros::tef::node::node_type::string:
            return "string";
        case tavros::tef::node::node_type::integer:
            return "integer";
        case tavros::tef::node::node_type::floating_point:
            return "floating-point";
        case tavros::tef::node::node_type::boolean:
            return "boolean";
        default:
            TAV_UNREACHABLE();
        }
    }


    using path_set = tavros::core::unordered_set<tavros::core::string, tavros::core::string_hash, tavros::core::string_equal>;
    using inheritance_t = tavros::core::vector<tavros::tef::parse_result::inherit_proto_t>;


    struct loader_impl
    {
        using string = tavros::core::string;
        using string_view = tavros::core::string_view;
        using node = tavros::tef::node;
        using small_string = tavros::core::fixed_string<512>;

        tavros::tef::source_provider& provider;
        string&                       errors;
        tavros::tef::registry&        reg;

        uint32 total_errors = 0;

        path_set visited;
        path_set loaded;

        inheritance_t inheritance;


        void report_error(string_view path, string_view code, string_view msg)
        {
            errors.append(small_string::format("{}: error: {}: {}\n", path, code, msg));
            ++total_errors;
        }

        void report_resolve_error(const tavros::tef::parse_result::inherit_proto_t& inh, string_view code, string_view msg)
        {
            if (!inh.file.empty()) {
                errors.append(inh.file);
                errors.append(":");
            }
            errors.append(small_string::format("{}:{}: error: {}: {}\n", inh.row, inh.col, code, msg));
            ++total_errors;
        }

        string load_source(string_view path) noexcept
        {
            try {
                return provider.load(path);
            } catch (const tavros::core::file_error& e) {
                report_error(path, "E-14", e.what());
            } catch (const std::exception& e) {
                report_error(path, "E-14", e.what());
            } catch (...) {
                report_error(path, "E-14", "Unknown error while reading file");
            }
            return {};
        }

        string resolve_path(string_view /*current_path*/, tavros::core::string_view include_path)
        {
            // Currently returns include_path as-is.
            // Path resolution relative to current_path can be added here later.
            return tavros::core::string(include_path);
        }

        void load(string_view path, node* pos)
        {
            if (loaded.count(path)) {
                // Already loaded
                return;
            }

            if (visited.count(path)) {
                report_error(path, "E-15", "Cycle detected in include graph");
                return;
            }

            auto source = load_source(path);
            if (source.empty()) {
                return;
            }

            node* doc = reg.new_document(path, pos);
            visited.insert(tavros::core::string(path));

            tavros::tef::parse_result result = tavros::tef::parser::parse(source, *doc, errors);
            source = {}; // Source no longer needed

            total_errors += result.number_of_errors;

            if (!result.inheritance.empty()) {
                inheritance.append_range(result.inheritance);
            }

            // Process includes - insert each before the current doc so that
            // base definitions appear before derived ones in registry order.
            for (const auto& include_path : result.inclusions) {
                auto resolved_path = resolve_path(path, include_path);

                load(resolved_path, doc);
            }

            // File loaded
            visited.erase(path);
            loaded.insert(tavros::core::string(path));
        }

        static constexpr uint32 e_cycle_detected = 13;
        static constexpr uint32 e_chain_too_long = 19;

        // Detects cycle in the prototype chain starting from 'start'.
        // Returns error number (e_cycle_detected, e_chain_too_long) if a
        // cycle is found. 0 - no cycle.
        uint32 check_cycle(const node* start) const noexcept
        {
            tavros::core::fixed_vector<const node*, tavros::tef::registry::k_max_proto_depth> visited;

            const node* current = start;
            while (current != nullptr) {
                for (auto* v : visited) {
                    if (v == current) {
                        return e_cycle_detected;
                    }
                }
                if (visited.size() == visited.max_size()) {
                    return e_chain_too_long;
                }
                visited.push_back(current);
                current = current->prototype();
            }

            return 0;
        }

        bool has_direct_parrent(node* n, node* check) noexcept
        {
            while (n) {
                if (n == check) {
                    return true;
                }
                n = n->parent();
            }
            return false;
        }

        void resolve_inheritance()
        {
            for (size_t i = 0; i < inheritance.size(); ++i) {
                const auto& inh = inheritance[i];
                auto        proto_path = tavros::core::string_view(inh.path);

                TAV_ASSERT(!proto_path.empty());

                auto* proto = reg.at_path(proto_path);
                if (!proto) {
                    report_resolve_error(
                        inh, "E-12",
                        small_string::format(
                            "prototype path '{}' does not refer to an existing node: "
                            "check that the prototype is defined and the path is correct",
                            proto_path
                        )
                    );
                    continue;
                }

                if (proto->is_scalar()) {
                    report_resolve_error(
                        inh, "E-10",
                        small_string::format(
                            "prototype path '{}' resolves to a scalar value of type '{}': "
                            "only object nodes may be used as prototypes",
                            proto_path, to_string(proto->type())
                        )
                    );
                    continue;
                }

                if (has_direct_parrent(inh.n, proto)) {
                    report_resolve_error(
                        inh, "E-20",
                        small_string::format(
                            "prototype path '{}' refers to the derived node itself or to "
                            "a structural ancestor: such prototype references are not allowed",
                            proto_path
                        )
                    );
                    continue;
                }

                tavros::tef::set_prototype(inh.n, proto);

                auto check_result = check_cycle(inh.n);
                if (check_result != 0) {
                    TAV_ASSERT(check_result == e_cycle_detected || check_result == e_chain_too_long);

                    // Undo the link
                    tavros::tef::set_prototype(inh.n, nullptr);

                    if (check_result == e_cycle_detected) {
                        report_resolve_error(
                            inh, "E-13",
                            small_string::format(
                                "setting prototype '{}' for node '{}' creates a cycle "
                                "in the prototype reference graph: "
                                "prototype chains must be acyclic",
                                proto_path,
                                inh.n->has_key() ? inh.n->key() : "<anonymous>"
                            )
                        );
                    } else {
                        report_resolve_error(
                            inh, "E-19",
                            tavros::core::fixed_string<512>::format(
                                "setting prototype '{}' for node '{}' exceeds the maximum "
                                "prototype chain depth of 32 levels",
                                proto_path,
                                inh.n->has_key() ? inh.n->key() : "<anonymous>"
                            )
                        );
                    }
                    continue;
                }
            }
        }
    };

} // namespace

namespace tavros::tef
{

    core::unique_ptr<registry> loader::load(core::string_view path)
    {
        core::string errors;
        auto         reg = load(path, errors);

        if (!errors.empty()) {
            logger.error("Error parsing file: {}\n{}", path, errors);
        }

        return reg;
    }

    core::unique_ptr<registry> loader::load(core::string_view path, core::string& errors)
    {
        auto        reg = core::make_unique<registry>();
        loader_impl ldr{*m_provider, errors, *reg};

        ldr.load(path, nullptr);
        ldr.resolve_inheritance();

        if (ldr.total_errors > 0) {
            errors.append(tavros::core::fixed_string<64>::format("Total of {} errors", ldr.total_errors));
        }

        return reg;
    }


} // namespace tavros::tef
