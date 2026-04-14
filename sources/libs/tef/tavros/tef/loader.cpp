#include <tavros/tef/loader.hpp>

#include <tavros/core/logger/logger.hpp>
#include <tavros/tef/parser.hpp>
#include <tavros/core/exception.hpp>
#include <tavros/core/utils/string_hash.hpp>

#include <string>

namespace
{
    tavros::core::logger logger("tef_loader");

    using path_set = tavros::core::unordered_set<tavros::core::string, tavros::core::string_hash, tavros::core::string_equal>;

    void append_error(
        tavros::core::string&     errors,
        tavros::core::string_view file,
        tavros::core::string_view message
    )
    {
        errors.append(file);
        errors.append(" : ");
        errors.append(message);
        errors.append("\n");
    }

    tavros::core::string resolve_path(
        tavros::core::string_view /*current_path*/,
        tavros::core::string_view include_path
    )
    {
        // Currently returns include_path as-is.
        // Path resolution relative to current_path can be added here later.
        return tavros::core::string(include_path);
    }

    tavros::core::string load_source(
        tavros::tef::tef_provider* provider,
        tavros::core::string_view  path,
        tavros::core::string&      errors
    )
    {
        TAV_ASSERT(provider);
        try {
            return provider->load(path);
        } catch (const tavros::core::file_error& e) {
            tavros::core::string msg = "[E-14] ";
            msg.append(e.what());
            append_error(errors, path, msg);
        } catch (const std::exception& e) {
            tavros::core::string msg = "[E-14] ";
            msg.append(e.what());
            append_error(errors, path, msg);
        } catch (...) {
            append_error(errors, path, "[E-14] Unknown error while reading file");
        }
        return {};
    }

    bool load_file(
        tavros::tef::tef_provider* provider,
        tavros::core::string_view  path,
        tavros::tef::node*         pos,
        tavros::tef::registry&     reg,
        tavros::core::string&      errors,
        path_set&                  visited,
        path_set&                  loaded
    )
    {
        if (loaded.count(path)) {
            // Already loaded
            return true;
        }

        if (visited.count(path)) {
            append_error(errors, path, "[E-15] Cycle detected in include graph ");
            return false;
        }

        auto source = load_source(provider, path, errors);
        if (source.empty()) {
            return false;
        }

        tavros::tef::node* doc = reg.new_document(path, pos);
        visited.insert(tavros::core::string(path));

        tavros::tef::parse_result result = tavros::tef::parser::parse(source, *doc, errors);
        source = {}; // Source no longer needed

        bool ok = result.success;

        // Process includes - insert each before the current doc so that
        // base definitions appear before derived ones in registry order.
        for (const auto& include_path : result.inclusions) {
            const tavros::core::string resolved = resolve_path(path, include_path);

            const bool include_ok = load_file(
                provider,
                resolved,
                doc,
                reg,
                errors,
                visited,
                loaded
            );

            if (!include_ok) {
                ok = false;
                // Continue to accumulate all errors
            }
        }

        // File loaded
        visited.erase(path);
        loaded.insert(tavros::core::string(path));

        return ok;
    }
} // namespace

namespace tavros::tef
{

    core::unique_ptr<registry> loader::load(core::string_view path)
    {
        core::string errors;
        auto         reg = load(path, errors);

        if (!errors.empty()) {
            logger.error("Error reading file: {}\n{}", path, errors);
        }

        return reg;
    }

    core::unique_ptr<registry> loader::load(core::string_view path, core::string& errors)
    {
        auto reg = core::make_unique<registry>();

        path_set visited;
        path_set loaded;

        load_file(m_provider.get(), path, nullptr, *reg, errors, visited, loaded);

        return reg;
    }


} // namespace tavros::tef
